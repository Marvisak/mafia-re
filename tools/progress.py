"""Draw the README's progress cards, one SVG per target binary, and write each target's score list.

The numbers count each binary's own functions and leave the CRT out, as reccmp's --nolib does:

- Implemented: the functions we have source for, out of all functions in the original.
- Accuracy: reccmp's average score over those functions.
- Progress (the bar, and the number next to the binary's name): the scores summed over every function
  of the original, so a function without source counts as 0.

The score list (docs/progress/<name>.txt, next to the card) has a line for every function and vtable reccmp knows,
by the original's address: reccmp's score, `100.00%*` for equivalent code, `stub` for a STUB marker, `missing` where
no function of ours matched. Committed with the cards, it shows each change's effect function by function:
the script prints what got worse than in the list it replaces (and what was dropped from it), and `git diff
docs/progress` shows the rest. The secondary setups have a list but no card.

It builds each target's preset first (vc6-pdb, vs2008 for Steam's setup), because reccmp reads the
PDB and a stale one gives wrong numbers.
Run it from the repository root and commit the cards and lists with the change that moved them:

    .venv/Scripts/python tools/progress.py                 # every target
    .venv/Scripts/python tools/progress.py GAME LS3DF      # some targets
    .venv/Scripts/python tools/progress.py GAME --check    # compare with the lists, write nothing
"""

import argparse
import logging
import re
import subprocess
from dataclasses import dataclass
from pathlib import Path
from xml.sax.saxutils import escape

from reccmp.compare import Compare
from reccmp.compare.report import ReccmpComparedEntity, ReccmpStatusReport, report_function_accuracy
from reccmp.project.detect import RecCmpProject
from reccmp.types import EntityType

REPO = Path(__file__).resolve().parent.parent
OUT = REPO / "docs" / "progress"


@dataclass
class Target:
    binary: str  # the card's title
    reccmp_id: str | None  # None until the binary has a reccmp target
    functions: int | None  # functions in the original's own code; None: no card, only the score list
    preset: str = "vc6-pdb"  # the build reccmp reads, in build/<preset>
    stem: str | None = None  # the card's and the list's file name in docs/progress, if not the binary's

    @property
    def card_path(self) -> Path:
        return OUT / f"{self.stem or Path(self.binary).stem}.svg"

    @property
    def list_path(self) -> Path:
        return OUT / f"{self.stem or Path(self.binary).stem}.txt"


# The function counts come from Binary Ninja: its functions from the start of .text up to the first import
# thunk or CRT function (the address in the comment). rw_data's, IJoy's and the three setups' are exact.
# Game.exe's still includes Binary Ninja's mistakes (padding and pieces of functions taken for functions); correct it
# as that binary is worked on. Once reccmp knows more functions than the count here, its number is used.
# LS3DF's is exact and counts the engine only, up to the libraries linked after it (dxerr8, S3TC, the DirectShow
# base classes), as --nolib does; so are the base classes' adjustor thunks and IUnknown members in IShow.cpp's
# object. Checked against reccmp's functions one by one: Binary Ninja's 1364 there (5 of them start in the padding
# before the function), less 2 pieces of IsCPUID's __except, 2 runs of padding and 4 of the base classes'
# functions, plus 5 that Binary Ninja misses (IGraph::EndScene, ITexture::Preload, two $E and the destructor
# stream_shows registers, 0x100758c0).
# Patch 1.1's LS3DF is counted the same way: Binary Ninja's 1357 up to d3d8's thunk (5 of them start in the padding
# before the function), less 2 pieces of IsCPUID's __except, 2 runs of padding and 3 of the base classes' functions
# (CBaseVideoRenderer's IUnknown members), plus the same 5 it misses. The particle base's scalar deleting destructor
# (0x1003ea30) is the engine's, as in 1.2, though CRendererInputPin's has the same bytes.
TARGETS = [
    Target("rw_data.dll", "RWDATA", 62),  # 0x10005cc0
    Target("setup.exe", "SETUP", 15),  # 0x403110
    Target("LS3DF.dll", "LS3DF", 1361),  # 0x1007d9d0
    Target("LS3DF.dll (patch 1.1)", "LS3DF11", 1355, stem="LS3DF.1.1"),  # 0x1007a800
    Target("Game.exe", "GAME", 5037),  # 0x60fc30, in Game.orig.exe
    Target("IJoy.dll", "IJOY", 97),  # 0x100060f0
    Target("Setup.1.0.exe", "SETUP10", None, stem="setup.1.0"),
    Target("setup.polish.exe", "SETUPPL", None, stem="setup.polish"),
    Target("setup.czech.exe", "SETUPCZ", None, stem="setup.czech"),
    Target("setup.steam.exe", "SETUPSTEAM", None, preset="vs2008", stem="setup.steam"),
]

BAR_WIDTH = 352

# GitHub's light and dark colours. An SVG shown through <img> follows the system's colour scheme, not
# GitHub's theme, so the card draws its own background and stays readable in either.
CARD = """\
<svg xmlns="http://www.w3.org/2000/svg" width="400" height="124" viewBox="0 0 400 124" role="img" aria-label="{label}">
<style>
  .card {{ fill: #ffffff; stroke: #d0d7de; }}
  .track {{ fill: #eaeef2; }}
  .bar {{ fill: #1a7f37; }}
  text {{ font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Helvetica, Arial, sans-serif; fill: #1f2328; font-variant-numeric: tabular-nums; }}
  .muted {{ fill: #59636e; }}
  @media (prefers-color-scheme: dark) {{
    .card {{ fill: #0d1117; stroke: #3d444d; }}
    .track {{ fill: #262c36; }}
    .bar {{ fill: #3fb950; }}
    text {{ fill: #f0f6fc; }}
    .muted {{ fill: #9198a1; }}
  }}
</style>
<rect class="card" x="4.5" y="4.5" width="391" height="115" rx="6"/>
<text x="24" y="38" font-size="18" font-weight="600">{name}</text>
<text x="376" y="38" font-size="18" font-weight="600" text-anchor="end">{progress}</text>
<rect class="track" x="24" y="50" width="{bar_width}" height="8" rx="4"/>
<rect class="bar" x="24" y="50" width="{bar:.2f}" height="8" rx="4"/>
<text class="muted" x="24" y="86" font-size="13">Implemented</text>
<text x="376" y="86" font-size="13" text-anchor="end">{implemented}</text>
<text class="muted" x="24" y="106" font-size="13">Accuracy</text>
<text x="376" y="106" font-size="13" text-anchor="end">{accuracy}</text>
</svg>
"""


def compare(project: RecCmpProject, target_id: str) -> ReccmpStatusReport:
    """reccmp's report of the target, filtered the way reccmp-reccmp --nolib filters it."""
    target = project.get(target_id)
    ignored = set(target.report_config.ignore_functions)

    def keep(entity) -> bool:
        if entity.get("library"):
            return False
        return not (entity.entity_type == EntityType.FUNCTION and entity.name in ignored)

    report = Compare.from_target(target).to_report(filename=target.original_path.name, filter_fn=keep)
    report.update_function_count()
    return report


def card(name: str, implemented: int, matched: float, total: int) -> str:
    progress = matched / total
    return CARD.format(
        label=escape(f"{name}: {progress:.2%} decompiled"),
        name=escape(name),
        progress=f"{progress:.2%}",
        bar_width=BAR_WIDTH,
        bar=progress * BAR_WIDTH,
        implemented=f"{implemented / total:.2%} ({implemented:,} / {total:,})",
        accuracy=f"{matched / implemented:.2%}" if implemented else "—",
    )


# A score list's line: the original's address, the status, then the name (vtables' prefixed with "vtable").
LINE = re.compile(r"^(0x[0-9a-f]+)\s+(\S+)\s+(.*)$")


def status(entity: ReccmpComparedEntity) -> str:
    if not entity.is_matched():
        return "missing"
    if entity.is_stub:
        return "stub"
    if entity.accuracy == 1.0:
        return "100.00%"
    if entity.is_effective_match:
        return "100.00%*"
    return f"{entity.accuracy:.2%}"


def rank(text: str) -> tuple[int, float]:
    """Orders the statuses: missing, stub, a score, equivalent, identical."""
    if text == "missing":
        return (0, 0.0)
    if text == "stub":
        return (1, 0.0)
    if text.endswith("*"):
        return (3, 1.0)
    value = float(text.rstrip("%"))
    return (4, 1.0) if value == 100.0 else (2, value)


def score_list(target: Target, report: ReccmpStatusReport) -> str:
    lines = [
        f"# {target.reccmp_id}: reccmp's score of each function and vtable of {target.binary} (--nolib), by the"
        " original's address.",
        "# Written by tools/progress.py. `100.00%*` is equivalent code, `stub` a STUB marker, `missing` no"
        " function of ours matched.",
    ]
    for addr in sorted(report.entities):
        entity = report.entities[addr]
        name = f"vtable {entity.name}" if entity.type == EntityType.VTABLE else entity.name
        lines.append(f"{addr:#010x}  {status(entity):<9} {name}")
    return "\n".join(lines) + "\n"


def parse_list(text: str) -> dict[str, tuple[str, str]]:
    """address -> (status, name)."""
    entries = {}
    for line in text.splitlines():
        m = LINE.match(line)
        if m:
            entries[m.group(1)] = (m.group(2), m.group(3))
    return entries


def changes(old: dict[str, tuple[str, str]], new: dict[str, tuple[str, str]]) -> None:
    """Print what got worse from the old list to the new one and count the rest."""
    worse, better, added = [], 0, 0
    for addr, (text, name) in new.items():
        if addr not in old:
            added += 1
        elif rank(text) < rank(old[addr][0]):
            worse.append(f"    {addr}  {old[addr][0]:>8} -> {text:<8}  {name}")
        elif rank(text) > rank(old[addr][0]):
            better += 1
    dropped = [f"    {addr}  {text:>8} -> gone      {name}" for addr, (text, name) in old.items() if addr not in new]
    for line in worse + dropped:
        print(line)
    print(f"    {len(worse)} worse, {len(dropped)} gone, {better} better, {added} new")


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__.split("\n", 1)[0])
    parser.add_argument("targets", nargs="*", metavar="TARGET", help="reccmp target ids (default: all)")
    parser.add_argument("--check", action="store_true", help="compare with the score lists and write nothing")
    args = parser.parse_args()

    known = {t.reccmp_id for t in TARGETS if t.reccmp_id}
    unknown = set(args.targets) - known
    if unknown:
        parser.error(f"unknown targets: {', '.join(sorted(unknown))} (known: {', '.join(sorted(known))})")
    targets = [t for t in TARGETS if not args.targets or t.reccmp_id in args.targets]

    presets = dict.fromkeys(t.preset for t in targets if t.reccmp_id)
    for preset in presets:
        if not (REPO / "build" / preset).exists():
            subprocess.run(["cmake", "--preset", preset], cwd=REPO, check=True)
        subprocess.run(["cmake", "--build", "--preset", preset], cwd=REPO, check=True)

    logging.basicConfig(level=logging.ERROR)
    projects = {preset: RecCmpProject.from_directory(REPO / "build" / preset) for preset in presets}
    OUT.mkdir(parents=True, exist_ok=True)
    for target in targets:
        implemented, matched, total = 0, 0.0, target.functions or 0
        report = None
        if target.reccmp_id:
            report = compare(projects[target.preset], target.reccmp_id)
            implemented, _, matched = report_function_accuracy(report)
            total = max(total, report.function_count)

        written = []
        if target.functions is not None:
            if not args.check:
                target.card_path.write_text(card(target.binary, implemented, matched, total), encoding="utf-8",
                                            newline="\n")
            written.append(target.card_path.relative_to(REPO).as_posix())
        print(f"{target.binary:22} {implemented:5} / {total:5} implemented, progress {matched / total:7.2%}"
              + ("" if args.check else f"  -> {', '.join(written + [target.list_path.relative_to(REPO).as_posix()])}"))

        if report is None:
            continue
        text = score_list(target, report)
        if target.list_path.exists():
            changes(parse_list(target.list_path.read_text(encoding="utf-8")), parse_list(text))
        if not args.check:
            target.list_path.write_text(text, encoding="utf-8", newline="\n")


if __name__ == "__main__":
    main()
