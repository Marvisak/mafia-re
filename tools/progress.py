"""Draw the README's progress cards: one SVG per target binary, with reccmp's percentages.

The numbers count each binary's own functions and leave the CRT out, as reccmp's --nolib does:

- Implemented: the functions we have source for, out of all functions in the original.
- Accuracy: reccmp's average score over those functions.
- Progress (the bar, and the number next to the binary's name): the scores summed over every function
  of the original, so a function without source counts as 0.

It builds each card's preset first (vc6-pdb, and vs2008 for Steam's setup.exe), because reccmp reads the
PDB and a stale one gives wrong numbers.
Run it from the repository root and commit the cards with the change that moved them:

    .venv/Scripts/python tools/progress.py
"""

import logging
import subprocess
from dataclasses import dataclass
from pathlib import Path
from xml.sax.saxutils import escape

from reccmp.compare import Compare
from reccmp.compare.report import report_function_accuracy
from reccmp.project.detect import RecCmpProject
from reccmp.types import EntityType

REPO = Path(__file__).resolve().parent.parent
OUT = REPO / "docs" / "progress"


@dataclass
class Target:
    binary: str  # the card's title
    reccmp_id: str | None  # None until the binary has a reccmp target
    functions: int  # functions in the original's own code
    preset: str = "vc6-pdb"  # the build reccmp reads, in build/<preset>
    svg: str | None = None  # the card's file name in docs/progress, if not the binary's

    @property
    def path(self) -> Path:
        return OUT / f"{self.svg or Path(self.binary).stem}.svg"


# The function counts come from Binary Ninja: its functions from the start of .text up to the first import
# thunk or CRT function (the address in the comment). rw_data's, IJoy's and both setups' are exact. The others
# still include Binary Ninja's mistakes (padding and pieces of functions taken for functions); correct them as
# those binaries are worked on. Once reccmp knows more functions than the count here, its number is used.
TARGETS = [
    Target("rw_data.dll", "RWDATA", 62),  # 0x10005cc0
    Target("setup.exe", "SETUP", 15),  # 0x403110
    Target("LS3DF.dll", "LS3DF", 1660),  # 0x1008d3e0
    Target("Game.exe", None, 5037),  # 0x60fc30, in Game.orig.exe
    Target("IJoy.dll", "IJOY", 97),  # 0x100060f0
    Target("setup.exe (Steam 2010)", "SETUPSTEAM", 16, "vs2008", "setup.steam"),  # 0x403158
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


def score(project: RecCmpProject, target_id: str) -> tuple[int, float, int]:
    """The implemented functions, their summed score and the number of functions reccmp knows, counted
    the way reccmp-reccmp --nolib counts them."""
    target = project.get(target_id)
    ignored = set(target.report_config.ignore_functions)

    def keep(entity) -> bool:
        if entity.get("library"):
            return False
        return not (entity.entity_type == EntityType.FUNCTION and entity.name in ignored)

    report = Compare.from_target(target).to_report(filename=target.original_path.name, filter_fn=keep)
    implemented, _, matched = report_function_accuracy(report)
    report.update_function_count()
    return implemented, matched, report.function_count


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


def main() -> None:
    presets = dict.fromkeys(t.preset for t in TARGETS if t.reccmp_id)
    for preset in presets:
        if not (REPO / "build" / preset).exists():
            subprocess.run(["cmake", "--preset", preset], cwd=REPO, check=True)
        subprocess.run(["cmake", "--build", "--preset", preset], cwd=REPO, check=True)

    logging.basicConfig(level=logging.ERROR)
    projects = {preset: RecCmpProject.from_directory(REPO / "build" / preset) for preset in presets}
    OUT.mkdir(parents=True, exist_ok=True)
    for target in TARGETS:
        implemented, matched, total = 0, 0.0, target.functions
        if target.reccmp_id:
            implemented, matched, known = score(projects[target.preset], target.reccmp_id)
            total = max(total, known)
        target.path.write_text(card(target.binary, implemented, matched, total), encoding="utf-8", newline="\n")
        print(f"{target.binary:22} {implemented:5} / {total:5} implemented, progress {matched / total:7.2%}"
              f"  -> {target.path.relative_to(REPO).as_posix()}")


if __name__ == "__main__":
    main()
