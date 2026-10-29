// The actors (PS2 C_actor.cpp, which has InitAnimIndex; the PC's file is most likely the one at 0x404ce0-0x40938c,
// docs/game-files.md). Written so far: the animations' table and their index, which InitSystem builds and CloseSystem
// frees, the constructor with global_actors and AddActorToGlobal, Init, which C_mission::OpenBin calls for the
// mission's actors, the destructor with DelActorFromGlobal, IsGlobalActor (PC only), which C_cutscene::Close calls,
// GameInit and GameDone, which C_game::Init and Done call
// (with DebustizeFrame and NoShadowFrame), SetActState and Rebirth, and rnd's float one, which the script calls with
// Tick, GetWorldDir, AngleTo2 and RotateDir (stubs). Not written: PS2's InternalPlacementReset (0x405e60) and
// ResetPlacement (0x405ee0), between Init and GameInit, which save and restore the frame's placement as GameInit and
// GameDone do. GetAnimIndex (0x408760, after DoneAnimIndex), the index's binary search, isn't called by anything
// written yet. The other virtual functions, which the actors' classes need, are stubs, in the vtable's addresses'
// order; seven of those addresses lie in other files' ranges, where the linker kept a copy (inline functions most
// likely, or folded with another class's), and the empty ones but DuplicateFrom's aren't annotated (folded with the
// other empty functions of their kinds). Release (0x4091d0) is inline (C_actor.h), its copy after DoneAnimIndex.

#include "actors/C_actor.h"

#include <stdlib.h>

#include "mission/Mission.h"
#include "mission/game.h"
#include "mission/C_cutscene.h"
#include "stub.h"

// The file's only initialized data, and its literals come before its functions'. An id is the animation in its low nine
// bits with at most one bit above them for its set: the eight sets of breathing, running and walking, the corner
// animations' weapons and stances.
// GLOBAL: GAME 0x62e488
S_anim_file _anim_info_[] = {
    {"breath01a.i3d", 0x0001},
    {"breath01b.i3d", 0x0002},
    {"breath01c.i3d", 0x0003},
    {"breath01d.i3d", 0x0004},
    {"run1.i3d", 0x0005},
    {"walk1.i3d", 0x0006},
    {"back1.i3d", 0x0007},
    {"left1.i3d", 0x0008},
    {"right1.i3d", 0x0009},
    {"turn1.i3d", 0x000a},
    {"strafL1.i3d", 0x000b},
    {"strafR1.i3d", 0x000c},
    {"strafRL1.i3d", 0x000d},
    {"strafRR1.i3d", 0x000e},
    {"runL1.i3d", 0x000f},
    {"runR1.i3d", 0x0010},
    {"walkL1.i3d", 0x0011},
    {"walkR1.i3d", 0x0012},
    {"backL1.i3d", 0x0013},
    {"backR1.i3d", 0x0014},
    {"!plachteni.i3d", 0x0066},
    {"!doskok.i3d", 0x0067},
    {"breath02a.i3d", 0x0201},
    {"breath01b.i3d", 0x0202},
    {"breath02a.i3d", 0x0203},
    {"breath01c.i3d", 0x0204},
    {"run2.i3d", 0x0205},
    {"walk2.i3d", 0x0206},
    {"back2.i3d", 0x0207},
    {"left2.i3d", 0x0208},
    {"right2.i3d", 0x0209},
    {"turn2.i3d", 0x020a},
    {"strafL2.i3d", 0x020b},
    {"strafR2.i3d", 0x020c},
    {"strafRL2.i3d", 0x020d},
    {"strafRR2.i3d", 0x020e},
    {"runL2.i3d", 0x020f},
    {"runR2.i3d", 0x0210},
    {"walkL2.i3d", 0x0211},
    {"walkR2.i3d", 0x0212},
    {"backL2.i3d", 0x0213},
    {"backR2.i3d", 0x0214},
    {"!plachteni.i3d", 0x0266},
    {"!doskok.i3d", 0x0267},
    {"breath03a.i3d", 0x0401},
    {"breath05b.i3d", 0x0402},
    {"breath05c.i3d", 0x0403},
    {"breath05d.i3d", 0x0404},
    {"run3.i3d", 0x0405},
    {"walk3.i3d", 0x0406},
    {"back3.i3d", 0x0407},
    {"left3.i3d", 0x0408},
    {"right3.i3d", 0x0409},
    {"turn3.i3d", 0x040a},
    {"strafL3.i3d", 0x040b},
    {"strafR3.i3d", 0x040c},
    {"strafRL3.i3d", 0x040d},
    {"strafRR3.i3d", 0x040e},
    {"runL3.i3d", 0x040f},
    {"runR3.i3d", 0x0410},
    {"walkL3.i3d", 0x0411},
    {"walkR3.i3d", 0x0412},
    {"backL3.i3d", 0x0413},
    {"backR3.i3d", 0x0414},
    {"!plachteni.i3d", 0x0466},
    {"!doskok.i3d", 0x0467},
    {"breath04a.i3d", 0x0801},
    {"breath05b.i3d", 0x0802},
    {"breath05c.i3d", 0x0803},
    {"breath05d.i3d", 0x0804},
    {"run4.i3d", 0x0805},
    {"walk4.i3d", 0x0806},
    {"back4.i3d", 0x0807},
    {"left4.i3d", 0x0808},
    {"right4.i3d", 0x0809},
    {"turn4.i3d", 0x080a},
    {"strafL4.i3d", 0x080b},
    {"strafR4.i3d", 0x080c},
    {"strafRL4.i3d", 0x080d},
    {"strafRR4.i3d", 0x080e},
    {"runL4.i3d", 0x080f},
    {"runR4.i3d", 0x0810},
    {"walkL4.i3d", 0x0811},
    {"walkR4.i3d", 0x0812},
    {"backL4.i3d", 0x0813},
    {"backR4.i3d", 0x0814},
    {"!plachteni.i3d", 0x0866},
    {"!doskok.i3d", 0x0867},
    {"breath05a.i3d", 0x1001},
    {"breath05b.i3d", 0x1002},
    {"breath05c.i3d", 0x1003},
    {"breath05d.i3d", 0x1004},
    {"run5.i3d", 0x1005},
    {"walk5.i3d", 0x1006},
    {"back5.i3d", 0x1007},
    {"left5.i3d", 0x1008},
    {"right5.i3d", 0x1009},
    {"turn5.i3d", 0x100a},
    {"strafL5.i3d", 0x100b},
    {"strafR5.i3d", 0x100c},
    {"strafRL5.i3d", 0x100d},
    {"strafRR5.i3d", 0x100e},
    {"runL5.i3d", 0x100f},
    {"runR5.i3d", 0x1010},
    {"walkL5.i3d", 0x1011},
    {"walkR5.i3d", 0x1012},
    {"backL5.i3d", 0x1013},
    {"backR5.i3d", 0x1014},
    {"!plachteni.i3d", 0x1066},
    {"!doskok.i3d", 0x1067},
    {"breath06a.i3d", 0x2001},
    {"breath06b.i3d", 0x2002},
    {"breath06c.i3d", 0x2003},
    {"breath06d.i3d", 0x2004},
    {"run6.i3d", 0x2005},
    {"walk6.i3d", 0x2006},
    {"back6.i3d", 0x2007},
    {"turn6.i3d", 0x200a},
    {"left6.i3d", 0x2008},
    {"right6.i3d", 0x2009},
    {"strafL6.i3d", 0x200b},
    {"strafR6.i3d", 0x200c},
    {"strafRL6.i3d", 0x200d},
    {"strafRR6.i3d", 0x200e},
    {"runL6.i3d", 0x200f},
    {"runR6.i3d", 0x2010},
    {"walkL6.i3d", 0x2011},
    {"walkR6.i3d", 0x2012},
    {"backL6.i3d", 0x2013},
    {"backR6.i3d", 0x2014},
    {"!plachteni.i3d", 0x2066},
    {"!doskok.i3d", 0x2067},
    {"breath07a.i3d", 0x4001},
    {"breath07b.i3d", 0x4002},
    {"breath07c.i3d", 0x4003},
    {"breath07d.i3d", 0x4004},
    {"run7.i3d", 0x4005},
    {"walk7.i3d", 0x4006},
    {"back7.i3d", 0x4007},
    {"left7.i3d", 0x4008},
    {"right7.i3d", 0x4009},
    {"turn7.i3d", 0x400a},
    {"strafL7.i3d", 0x400b},
    {"strafR7.i3d", 0x400c},
    {"strafRL7.i3d", 0x400d},
    {"strafRR7.i3d", 0x400e},
    {"runL7.i3d", 0x400f},
    {"runR7.i3d", 0x4010},
    {"walkL7.i3d", 0x4011},
    {"walkR7.i3d", 0x4012},
    {"backL7.i3d", 0x4013},
    {"backR7.i3d", 0x4014},
    {"!plachteni.i3d", 0x4066},
    {"!doskok.i3d", 0x4067},
    {"breath08a.i3d", 0x8001},
    {"breath08b.i3d", 0x8002},
    {"breath08c.i3d", 0x8003},
    {"breath08d.i3d", 0x8004},
    {"run8.i3d", 0x8005},
    {"walk8.i3d", 0x8006},
    {"back8.i3d", 0x8007},
    {"left8.i3d", 0x8008},
    {"right8.i3d", 0x8009},
    {"turn8.i3d", 0x800a},
    {"strafL8.i3d", 0x800b},
    {"strafR8.i3d", 0x800c},
    {"strafRL8.i3d", 0x800d},
    {"strafRR8.i3d", 0x800e},
    {"runL8.i3d", 0x800f},
    {"runR8.i3d", 0x8010},
    {"walkL8.i3d", 0x8011},
    {"walkR8.i3d", 0x8012},
    {"backL8.i3d", 0x8013},
    {"backR8.i3d", 0x8014},
    {"!plachteni.i3d", 0x8066},
    {"!doskok.i3d", 0x8067},
    {"jump1.i3d", 0x005f},
    {"jumpR?1.i3d", 0x0060},
    {"jumpstat?.i3d", 0x0061},
    {"jumpL1.i3d", 0x0062},
    {"jumpR1.i3d", 0x0063},
    {"jumpL3.i3d", 0x0064},
    {"jumpR3.i3d", 0x0065},
    {"na bednu 06m.i3d", 0x0068},
    {"na bednu 1m.i3d", 0x0069},
    {"hod vrchem hod.i3d", 0x00a3},
    {"sbirani zezemne.i3d", 0x00a4},
    {"sbirani zurovne.i3d", 0x00a5},
    {"sbirani zvrchu.i3d", 0x00a6},
    {"NoseniMrtvolyTomNahozeni.i3d", 0x00a7},
    {"NoseniMrtvolyTomShozeni.i3d", 0x00a8},
    {"boj basb z LH.i3d", 0x00a9},
    {"boj basb z LS.i3d", 0x00aa},
    {"boj basb z RS.i3d", 0x00ab},
    {"boj basb z RH.i3d", 0x00ac},
    {"boj basb z RS.i3d", 0x00ad},
    {"boj HPT basb LH.i3d", 0x00ae},
    {"boj HPT basb LS.i3d", 0x00af},
    {"boj HPT basb RH.i3d", 0x00b0},
    {"boj HPT basb RS.i3d", 0x00b1},
    {"boj nuz bod.i3d", 0x00b2},
    {"boj nuz sek01.i3d", 0x00b4},
    {"boj nuz sek02.i3d", 0x00b3},
    {"boj nuz kombo.i3d", 0x00b5},
    {"boj HPT nuz bod.i3d", 0x00b6},
    {"boj HPT nuz sek01.i3d", 0x00b8},
    {"boj HPT nuz sek02.i3d", 0x00b7},
    {"boj box L direct.i3d", 0x00b9},
    {"boj box R hak hlava.i3d", 0x00ba},
    {"boj box L hak hlava.i3d", 0x00bb},
    {"boj box L hak spodek.i3d", 0x00bc},
    {"boj box R hak spodek.i3d", 0x00bd},
    {"boj box kombo.i3d", 0x00be},
    {"boj HPT box L direct.i3d", 0x00bf},
    {"boj HPT box R hak hlava.i3d", 0x00c0},
    {"boj HPT box L hak hlava.i3d", 0x00c1},
    {"boj HPT box L hak spodek.i3d", 0x00c2},
    {"boj HPT box R hak spodek.i3d", 0x00c3},
    {"enm roh stoj stat L.i3d", 0x004e},
    {"enm roh pis stoj stat L.i3d", 0x024e},
    {"enm roh puska stoj stat L.i3d", 0x044e},
    {"enm roh puska stoj stat L.i3d", 0x084e},
    {"enm roh puska stoj stat L.i3d", 0x104e},
    {"enm roh drep stat L.i3d", 0x204e},
    {"enm roh pis drep stat L.i3d", 0x404e},
    {"enm roh puska drep stat L.i3d", 0x804e},
    {"enm roh stoj stat R.i3d", 0x004f},
    {"enm roh pis stoj stat L.i3d", 0x024f},
    {"enm roh puska stoj stat R.i3d", 0x044f},
    {"enm roh puska stoj stat R.i3d", 0x084f},
    {"enm roh puska stoj stat R.i3d", 0x104f},
    {"enm roh drep stat R.i3d", 0x204f},
    {"enm roh pis drep stat R.i3d", 0x404f},
    {"enm roh puska drep stat R.i3d", 0x804f},
    {"enm roh stoj nahl L.i3d", 0x0050},
    {"enm roh pis stoj nahl L.i3d", 0x0250},
    {"enm roh puska stoj nahl L.i3d", 0x0450},
    {"enm roh puska stoj nahl L.i3d", 0x0850},
    {"enm roh puska stoj nahl L.i3d", 0x1050},
    {"enm roh drep nahl L.i3d", 0x2050},
    {"enm roh pis drep nahl L.i3d", 0x4050},
    {"enm roh puska drep nahl L.i3d", 0x8050},
    {"enm roh stoj nahl R.i3d", 0x0051},
    {"enm roh pis stoj nahl R.i3d", 0x0251},
    {"enm roh puska stoj nahl R.i3d", 0x0451},
    {"enm roh puska stoj nahl R.i3d", 0x0851},
    {"enm roh puska stoj nahl R.i3d", 0x1051},
    {"enm roh drep nahl R.i3d", 0x2051},
    {"enm roh pis drep nahl R.i3d", 0x4051},
    {"enm roh puska drep nahl R.i3d", 0x8051},
    {"enm roh drep nahl T.i3d", 0x2052},
    {"enm roh pis drep nahl T.i3d", 0x4052},
    {"enm roh puska drep nahl T.i3d", 0x8052},
    {"enm roh pis stoj fire01 L.i3d", 0x0253},
    {"enm roh brok stoj fire01 L.i3d", 0x0853},
    {"enm roh sap stoj fire01 L.i3d", 0x0854},
    {"enm roh brok stoj fire01 L.i3d", 0x1053},
    {"enm roh sap stoj fire01 L.i3d", 0x1054},
    {"enm roh pis drep fire01 L.i3d", 0x4053},
    {"enm roh puska drep fire01 L.i3d", 0x8053},
    {"enm roh pis stoj fire01 R.i3d", 0x0255},
    {"enm roh brok stoj fire01 R.i3d", 0x0855},
    {"enm roh sap stoj fire01 R.i3d", 0x0856},
    {"enm roh brok stoj fire01 R.i3d", 0x1055},
    {"enm roh sap stoj fire01 R.i3d", 0x1056},
    {"enm roh pis drep fire01 R.i3d", 0x4055},
    {"enm roh puska drep fire01 R.i3d", 0x8055},
    {"enm roh pis drep fire01 T.i3d", 0x4057},
    {"enm roh puska drep fire01 T.i3d", 0x8057},
    {"enm roh pis stoj fire02 L.i3d", 0x0258},
    {"enm roh brok stoj fire02 L.i3d", 0x0858},
    {"enm roh sap stoj fire02 L.i3d", 0x0859},
    {"enm roh brok stoj fire02 L.i3d", 0x1058},
    {"enm roh sap stoj fire02 L.i3d", 0x1059},
    {"enm roh pis drep fire02 L.i3d", 0x4058},
    {"enm roh puska drep fire02 L.i3d", 0x8058},
    {"enm roh pis stoj fire02 R.i3d", 0x025a},
    {"enm roh brok stoj fire02 R.i3d", 0x085a},
    {"enm roh sap stoj fire02 R.i3d", 0x085b},
    {"enm roh brok stoj fire02 R.i3d", 0x105a},
    {"enm roh sap stoj fire02 R.i3d", 0x105b},
    {"enm roh pis drep fire02 R.i3d", 0x405a},
    {"enm roh puska drep fire02 R.i3d", 0x805a},
    {"enm roh pis drep fire02 T.i3d", 0x405c},
    {"enm roh puska drep fire02 T.i3d", 0x805c},
    {"smrt pad dozadu.I3D", 0x006a},
    {"smrt pad vlevo.I3D", 0x006b},
    {"smrt pad vpravo.I3D", 0x006c},
    {"smrt pad end A.I3D", 0x006d},
    {"smrt pad end B.I3D", 0x006e},
    {"smrt pad end vstavani.I3D", 0x006f},
    {"smrt pad dopredu A.i3d", 0x0070},
    {"smrt pad dopredu B.i3d", 0x0071},
    {"smrt pad dopredu C.i3d", 0x0072},
    {"smrt stoj zada A.I3D", 0x0083},
    {"smrt stoj zada B.I3D", 0x0084},
    {"smrt stoj zada C.I3D", 0x0085},
    {"smrt stoj bricho A.i3d", 0x0086},
    {"smrt stoj bricho B.I3D", 0x0087},
    {"smrt stoj bricho C.I3D", 0x0088},
    {"smrt stoj hlava zezadu.i3d", 0x0089},
    {"smrt stoj hlava zepredu.I3D", 0x008a},
    {"smrt stoj hlava otocka.i3d", 0x008b},
    {"smrt drep zada A.I3D", 0x008c},
    {"smrt drep zada B.I3D", 0x008d},
    {"smrt drep bricho A.I3D", 0x008e},
    {"smrt drep bricho B.I3D", 0x008f},
    {"smrt drep hlava zezadu.I3D", 0x0090},
    {"smrt drep hlava zepredu.I3D", 0x0091},
    {"smrt drep hlava otocka.I3D", 0x0092},
    {"smrt stoj bricho T pad.I3D", 0x0093},
    {"smrt stoj bricho T plazeni.I3D", 0x0094},
    {"smrt stoj bricho T stativ.I3D", 0x0095},
    {"smrt stoj bricho T end.I3D", 0x0096},
    {"smrt stoj zada T pad.I3D", 0x0097},
    {"smrt stoj zada T plazeni.I3D", 0x0098},
    {"smrt stoj zada T stativ.I3D", 0x0099},
    {"smrt stoj zada T end.I3D", 0x009a},
    {"smrt drep bricho T pad.I3D", 0x009b},
    {"smrt drep bricho T plazeni.I3D", 0x009c},
    {"smrt drep bricho T stativ.I3D", 0x009d},
    {"smrt drep bricho T end.I3D", 0x009e},
    {"smrt drep zada T pad.I3D", 0x009f},
    {"smrt drep zada T plazeni.I3D", 0x00a0},
    {"smrt drep zada T stativ.I3D", 0x00a1},
    {"smrt drep zada T end.I3D", 0x00a2},
    {"zasah Sm rameno FR.i3d", 0x00c4},
    {"zasah Sm rameno BR.i3d", 0x00c5},
    {"zasah Sm rameno FL.i3d", 0x00c6},
    {"zasah Sm rameno BL.i3d", 0x00c7},
    {"zasah Sm noha R.i3d", 0x00c8},
    {"zasah Sm noha L.i3d", 0x00c9},
    {"zasah Sm bricho.i3d", 0x00ca},
    {"zasah Sm zada.i3d", 0x00cb},
    {"zasah Sm hlava F.i3d", 0x00cc},
    {"zasah Sm hlava B.i3d", 0x00cd},
    {"zasah Lg rameno FR.i3d", 0x00ce},
    {"zasah Lg rameno BR.i3d", 0x00cf},
    {"zasah Lg rameno FL.i3d", 0x00d0},
    {"zasah Lg rameno BL.i3d", 0x00d1},
    {"zasah Lg noha R.i3d", 0x00d2},
    {"zasah Lg noha L.i3d", 0x00d3},
    {"zasah Lg hlava F.i3d", 0x00cc},
    {"zasah Lg hlava B.i3d", 0x00cd},
    {"zasah Drep rameno FR.i3d", 0x00d6},
    {"zasah Drep rameno BR.i3d", 0x00d7},
    {"zasah Drep rameno FL.i3d", 0x00d8},
    {"zasah Drep rameno BL.i3d", 0x00d9},
    {"zasah Drep noha R.i3d", 0x00da},
    {"zasah Drep noha L.i3d", 0x00db},
    {"zasah Drep bricho.i3d", 0x00dc},
    {"zasah Drep zada.i3d", 0x00dd},
    {"zasah Sm Drep hlava F.i3d", 0x00de},
    {"zasah Sm Drep hlava B.i3d", 0x00df},
    {"zasah Lg Drep hlava F.i3d", 0x00e0},
    {"zasah Lg Drep hlava B.i3d", 0x00e1},
    {"zasah HPT nogunF.i3d", 0x00e2},
    {"zasah HPT nogunB.i3d", 0x00e3},
    {"zasah HPT pistol1F.i3d", 0x00e4},
    {"zasah HPT pistol1B.i3d", 0x00e5},
    {"zasah HPT puska1F.i3d", 0x00e6},
    {"zasah HPT puska1B.i3d", 0x00e7},
    {"zasah HPT sapikF.i3d", 0x00e8},
    {"zasah HPT sapikB.i3d", 0x00e9},
    {"zasah HPT puska2F.i3d", 0x00ea},
    {"zasah HPT puska2B.i3d", 0x00eb},
    {"boj zasah hl D.i3d", 0x00ec},
    {"boj zasah hl L.i3d", 0x00ed},
    {"boj zasah hl R.i3d", 0x00ee},
    {"boj zasah br L.i3d", 0x00ef},
    {"boj zasah br R.i3d", 0x00f0},
    {"boj box rozkrok.i3d", 0x00f1},
    {"boj zasah rozkrok sm.i3d", 0x00f2},
    {"boj box hlavicka.i3d", 0x00f3},
    {"boj zasah hlavicka.i3d", 0x00f4},
    {"KradeniAuta01.i3d", 0x00f5},
    {"AutoRidicStativ.i3d", 0x00f6},
    {"FormuleStativ.i3d", 0x00f7},
    {"AutoSpolStativ.i3d", 0x00f8},
    {"AutoRidicVolant.i3d", 0x00f9},
    {"FormuleVolant.i3d", 0x00fa},
    {"Auto fire01 L On.i3d", 0x00fb},
    {"Auto fire01 L Off.i3d", 0x00fc},
    {"Auto fire01 L.i3d", 0x00fd},
    {"Auto fire01 R On.i3d", 0x00fe},
    {"Auto fire01 R Off.i3d", 0x00ff},
    {"Auto fire01 R.i3d", 0x0100},
    {"Auto fire02 L On.i3d", 0x0101},
    {"Auto fire02 L Off.i3d", 0x0102},
    {"Auto fire02 L.i3d", 0x0103},
    {"Auto fire02 R On.i3d", 0x0104},
    {"Auto fire02 R Off.i3d", 0x0105},
    {"Auto fire02 R.i3d", 0x0106},
    {"Auto fire03 On.i3d", 0x0107},
    {"Auto fire03 Off.i3d", 0x0108},
    {"Auto fire03 L.i3d", 0x0109},
    {"smrt auto ridic01.i3d", 0x010a},
    {"smrt auto spol IN01.i3d", 0x010b},
    {"smrt auto spol IN02.i3d", 0x010c},
    {"smrt auto spol OutL.i3d", 0x010d},
    {"smrt auto spol OutR.i3d", 0x010e},
    {"AutoSmNas FL.i3d", 0x010f},
    {"AutoSmNas FR.i3d", 0x0110},
    {"AutoSmNas BL.i3d", 0x0111},
    {"AutoSmNas BR.i3d", 0x0112},
    {"AutoBigNas FL.i3d", 0x0113},
    {"AutoBigNas FR.i3d", 0x0114},
    {"AutoBigNas BL.i3d", 0x0115},
    {"AutoBigNas BR.i3d", 0x0116},
    {"AutoSmVys FL.i3d", 0x011f},
    {"AutoSmVys FR.i3d", 0x0120},
    {"AutoSmVys BL.i3d", 0x0121},
    {"AutoSmVys BR.i3d", 0x0122},
    {"AutoBigVys FL.i3d", 0x0123},
    {"AutoBigVys FR.i3d", 0x0124},
    {"AutoBigVys BL.i3d", 0x0125},
    {"AutoBigVys BR.i3d", 0x0126},
    {"Auto lezeni z vraku.i3d", 0x012f},
    {"AutoPrelezLtoR.i3d", 0x0130},
    {"AutoPrelezRtoL.i3d", 0x0131},
    {"VyhozManFrontL.i3d", 0x0132},
    {"VyhozManBackL.i3d", 0x0133},
    {"VyhozFrontL.i3d", 0x0134},
    {"VyhozFrontR.i3d", 0x0135},
    {"VyhozBackL.i3d", 0x0136},
    {"VyhozBackR.i3d", 0x0137},
    {"VyhozMrtFrontL.i3d", 0x0138},
    {"VyhozMrtFrontR.i3d", 0x0139},
    {"VyhozMrtBackL.i3d", 0x013a},
    {"VyhozMrtBackR.i3d", 0x013b},
    {"FormuleNastupL.i3d", 0x013c},
    {"FormuleVystupL.i3d", 0x013d},
    {"FormuleNastupR.i3d", 0x013e},
    {"FormuleVystupR.i3d", 0x013f},
    {"salina nastup.i3d", 0x0140},
    {"salina vystup.i3d", 0x0141},
    {"enm klid alarm L.i3d", 0x0073},
    {"enm klid alarm R.i3d", 0x0074},
    {"enm klid alarm F.i3d", 0x0075},
    {"enm klid alarm 180.i3d", 0x0076},
    {"enm klid puska alarm L.i3d", 0x007b},
    {"enm klid puska alarm R.i3d", 0x007c},
    {"enm klid puska alarm F.i3d", 0x007d},
    {"enm klid puska alarm 180.i3d", 0x007e},
    {"enm klid sapik alarm L.i3d", 0x007f},
    {"enm klid sapik alarm R.i3d", 0x0080},
    {"enm klid sapik alarm F.i3d", 0x0081},
    {"enm klid sapik alarm 180.i3d", 0x0082},
    {"Uskok Right Front.I3D", 0x0142},
    {"Uskok Left Front.I3D", 0x0143},
    {"Uskok Left Back.I3D", 0x0144},
    {"Uskok Right Back.I3D", 0x0145},
    {"Uskok Front Left.I3D", 0x0146},
    {"Uskok Back Left.I3D", 0x0147},
    {"Uskok Back Right.I3D", 0x0148},
    {"Uskok Front Right.I3D", 0x0149},
    {"x panika drep dolu.i3d", 0x014a},
    {"x panika drep stativ.i3d", 0x014b},
    {"x panika drep nahoru.i3d", 0x014c},
    {"x panika na4 dolu.i3d", 0x014d},
    {"x panika na4 stativ.i3d", 0x014e},
    {"x panika na4 nahoru.i3d", 0x014f},
    {"x panika leh dolu.i3d", 0x0150},
    {"x panika leh stativ.i3d", 0x0151},
    {"x panika leh nahoru.i3d", 0x0152},
    {"enm klid stat A.i3d", 0x0015},
    {"enm klid stat B.i3d", 0x0016},
    {"enm klid stat C.i3d", 0x0017},
    {"enm klid walk.i3d", 0x0018},
    {"enm klid back.i3d", 0x0019},
    {"enm klid left.i3d", 0x001a},
    {"enm klid right.i3d", 0x001b},
    {"enm klid strafL.i3d", 0x001c},
    {"enm klid strafR.i3d", 0x001d},
    {"enm klid walkL.i3d", 0x001e},
    {"enm klid walkR.i3d", 0x001f},
    {"enm klid backL.i3d", 0x0020},
    {"enm klid backR.i3d", 0x0021},
    {"enm klid puska stat A.i3d", 0x0415},
    {"enm klid puska stat B.i3d", 0x0416},
    {"enm klid puska stat C.i3d", 0x0417},
    {"enm klid puska walk.i3d", 0x0418},
    {"enm klid puska back.i3d", 0x0419},
    {"enm klid puska left.i3d", 0x041a},
    {"enm klid puska right.i3d", 0x041b},
    {"enm klid puska strafL.i3d", 0x041c},
    {"enm klid puska strafR.i3d", 0x041d},
    {"enm klid puska walkL.i3d", 0x041e},
    {"enm klid puska walkR.i3d", 0x041f},
    {"enm klid puska backL.i3d", 0x0420},
    {"enm klid puska backR.i3d", 0x0421},
    {"enm klid sapik stat A.i3d", 0x0815},
    {"enm klid sapik stat B.i3d", 0x0816},
    {"enm klid sapik stat C.i3d", 0x0817},
    {"enm klid sapik walk.i3d", 0x0818},
    {"enm klid sapik back.i3d", 0x0819},
    {"enm klid sapik left.i3d", 0x081a},
    {"enm klid sapik right.i3d", 0x081b},
    {"enm klid sapik strafL.i3d", 0x081c},
    {"enm klid sapik strafR.i3d", 0x081d},
    {"enm klid sapik walkL.i3d", 0x081e},
    {"enm klid sapik walkR.i3d", 0x081f},
    {"enm klid sapik backL.i3d", 0x0820},
    {"enm klid sapik backR.i3d", 0x0821},
    {"enm streh pis stat A.i3d", 0x0222},
    {"enm streh pis stat B.i3d", 0x0223},
    {"enm streh pis stat C.i3d", 0x0224},
    {"enm streh pis walk.i3d", 0x0225},
    {"enm streh pis back.i3d", 0x0226},
    {"enm streh pis left.i3d", 0x0227},
    {"enm streh pis right.i3d", 0x0228},
    {"enm streh pis strafL.i3d", 0x0229},
    {"enm streh pis strafR.i3d", 0x022a},
    {"enm streh pis walkL.i3d", 0x022b},
    {"enm streh pis walkR.i3d", 0x022c},
    {"enm streh pis backL.i3d", 0x022d},
    {"enm streh pis backR.i3d", 0x022e},
    {"enm streh puska stat A.i3d", 0x0422},
    {"enm streh puska stat B.i3d", 0x0423},
    {"enm streh puska stat C.i3d", 0x0424},
    {"enm streh puska walk.i3d", 0x0425},
    {"enm streh puska back.i3d", 0x0426},
    {"enm streh puska left.i3d", 0x0427},
    {"enm streh puska right.i3d", 0x0428},
    {"enm streh puska strafL.i3d", 0x0429},
    {"enm streh puska strafR.i3d", 0x042a},
    {"enm streh puska walkL.i3d", 0x042b},
    {"enm streh puska walkR.i3d", 0x042c},
    {"enm streh puska backL.i3d", 0x042d},
    {"enm streh puska backR.i3d", 0x042e},
    {"enm streh sapik stat A.i3d", 0x0822},
    {"enm streh sapik stat B.i3d", 0x0823},
    {"enm streh sapik stat C.i3d", 0x0824},
    {"enm streh sapik walk.i3d", 0x0825},
    {"enm streh sapik back.i3d", 0x0826},
    {"enm streh sapik left.i3d", 0x0827},
    {"enm streh sapik right.i3d", 0x0828},
    {"enm streh sapik strafL.i3d", 0x0829},
    {"enm streh sapik strafR.i3d", 0x082a},
    {"enm streh sapik walkL.i3d", 0x082b},
    {"enm streh sapik walkR.i3d", 0x082c},
    {"enm streh sapik backL.i3d", 0x082d},
    {"enm streh sapik backR.i3d", 0x082e},
    {"panika leknuti01L.i3d", 0x002f},
    {"panika leknuti01R.i3d", 0x0030},
    {"panika leknuti01F.i3d", 0x0031},
    {"panika leknuti01B.i3d", 0x0032},
    {"panika leknuti02L.i3d", 0x0033},
    {"panika leknuti02R.i3d", 0x0034},
    {"panika leknuti02F.i3d", 0x0035},
    {"panika leknuti02B.i3d", 0x0036},
    {"panika stoj stativ01a.i3d", 0x0037},
    {"panika stoj stativ01b.i3d", 0x0038},
    {"panika stoj stativ01c.i3d", 0x0039},
    {"panika stoj stativ02a.i3d", 0x003a},
    {"panika stoj stativ02b.i3d", 0x003b},
    {"panika stoj stativ02c.i3d", 0x003c},
    {"x panika drep dolu.i3d", 0x003d},
    {"x panika drep stativ.i3d", 0x003e},
    {"x panika drep nahoru.i3d", 0x003f},
    {"x panika na4 dolu.i3d", 0x0040},
    {"x panika na4 stativ.i3d", 0x0041},
    {"x panika na4 nahoru.i3d", 0x0042},
    {"x panika leh dolu.i3d", 0x0043},
    {"x panika leh stativ.i3d", 0x0044},
    {"x panika leh nahoru.i3d", 0x0045},
    {"panika run01a.i3d", 0x0046},
    {"panika run01b.i3d", 0x0047},
    {"panika run03a.i3d", 0x0048},
    {"panika run03b.i3d", 0x0049},
    {"panika run02a.i3d", 0x004a},
    {"panika run02b.i3d", 0x004b},
    {"panika rozhlizeni01.i3d", 0x004c},
    {"panika rozhlizeni02.i3d", 0x004d},
    {"pumpar.i3d", 0x0153},
    {"pumpar hadice.i3d", 0x0154},
    {NULL, 0xffff},
};

// The file's hashed global, before the zero-initialized ones
// GLOBAL: GAME 0x63d2c8
int C_actor::global_refs;

// Zero-initialized, so they follow the file's hashed globals in this order (their buckets would put the count first)
// GLOBAL: GAME 0x63d2cc
S_anim_index *_anim_index_ = NULL;
// GLOBAL: GAME 0x63d2d0
int _anim_index_count_ = 0;

#pragma auto_inline(off)

// STUB: GAME 0x404e00
void C_actor::vfunc_9()
{
    StubLog("C_actor::vfunc_9");
}

// STUB: GAME 0x404f30
void C_actor::vfunc_0()
{
    StubLog("C_actor::vfunc_0");
}

// STUB: GAME 0x404f40
void C_actor::vfunc_1()
{
    StubLog("C_actor::vfunc_1");
}

// STUB: GAME 0x405010
void C_actor::vfunc_2()
{
    StubLog("C_actor::vfunc_2");
}

// STUB: GAME 0x4054c0
void C_actor::vfunc_3()
{
    StubLog("C_actor::vfunc_3");
}

// STUB: GAME 0x405a00
void C_actor::vfunc_4()
{
    StubLog("C_actor::vfunc_4");
}

#pragma auto_inline(on)

// Its initializer (0x405c20) comes before the constructor. PS2's posscoll_frame, a vector too (0x63d2a4), follows it
// with its own (0x405ca0). Open: their destructors (0x405c60, 0x405ce0) test and set a guard byte (0x63d2b4) they share,
// a bit each, as C_program::change_progs' does.
// GLOBAL: GAME 0x63d2a4
std::vector<uint> C_actor::posscoll_frame;

// GLOBAL: GAME 0x63d2b8
std::vector<C_actor *> C_actor::global_actors;

// FUNCTION: GAME 0x405d20
C_actor::C_actor() :
    unknown_4(0),
    ref(1),
    type(ACTOR_ACTOR),
    unknown_14(1),
    unknown_20(false),
    unknown_60(0),
    unknown_66(false),
    frame(NULL)
{
    unknown_8 = 0;
    global_refs++;
    AddActorToGlobal(this);
}

// PS2's without its array at 0x48 and with the cutscene's (PC only), which may hold the actor
// FUNCTION: GAME 0x405d90
C_actor::~C_actor()
{
    global_refs--;
    cutscene.RemoveActor(this, false);
    if (own_frame && frame)
        frame->Release();
    DelActorFromGlobal(this);
}

// The frame's user data cleared, though it may point to the actor already
// FUNCTION: GAME 0x405dd0
bool C_actor::Init(I3D_frame *frm)
{
    if (!frm)
        return false;
    C_actor **p = GetFrameActor(frm);
    if (p && *p && *p != this)
        return false;
    frame = frm;
    frame->m_UserData = 0;
    C_mission *mission = act_mission;
    number = ++mission->unknown_8;
    own_frame = false;
    if (frame)
    {
        // STRING: GAME 0x632240
        I3D_frame *target = frame->FindChildFrame("targetE", ENUMF_TARGET);
        if (target)
            target->SetOn(false);
        // STRING: GAME 0x632238
        target = frame->FindChildFrame("targetN", ENUMF_TARGET);
        if (target)
            target->SetOn(false);
    }
    act_state = ACTSTATE_0;
    return true;
}

// Takes the actor into the game, at the mission's start (C_game::Init): the frame's placement saved for GameDone, the
// frame given its pointer to the actor unless it has one, the position and direction taken from it. As PS2's but for
// the types left in their place (PS2 leaves only ACTOR_ACTOR). The frame goes through a local where the original keeps
// it across calls (the user data's new, Make, UpdateWMatrixProc), as PS2's does. Open: the inlined GetDir's
// normalization sums the squares as y, z, x where the original's sums z, x, y (the out-of-line copy at 0x47a890 has
// ours), with the same instructions otherwise.
// FUNCTION: GAME 0x405fa0
void C_actor::GameInit()
{
    act_state = ACTSTATE_0;
    unknown_65 = type == ACTOR_ACTOR;
    if (frame)
    {
        // A car anywhere but in a sector goes into the primary sector, a metre higher
        if (type == ACTOR_CAR && frame->m_pParent && frame->m_pParent->GetType() != FRAME_SECTOR)
        {
            frame->LinkTo(act_mission->scene->primary_sector, I3DLINK_UPDMATRIX);
            act_mission->scene->SetFrameSectorPos(frame, frame->GetWorldPos() + S_vector(0.0f, 1.0f, 0.0f));
        }
        I3D_frame *frm = frame;
        if (GetFrameActor(frm) && *GetFrameActor(frm))
            frame_actor_set = false;
        else
        {
            frame_actor_set = true;
            C_actor **p = GetFrameActor(frm);
            if (!p)
            {
                p = new C_actor *;
                frm->m_UserData = (uint)p;
            }
            *p = this;
            SetFrameActor(frame, p);
        }
        saved_on = frame->IsOn();
        saved_pos = frame->GetPos();
        saved_rot = frame->GetRot();
        frm = frame;
        saved_parent = frm->m_pParent;
        // The actors but the plain ones and the shots: one in a sector is put in the sector of its position, a model
        // under another frame is linked to the sector above it
        if (type != ACTOR_ACTOR && type != ACTOR_ACTOR_SHOT && frm->m_pParent &&
            frm->m_pParent->GetType() == FRAME_SECTOR)
        {
            I3D_scene *scene = act_mission->scene;
            scene->SetFrameSectorPos(frm, frm->GetWorldPos());
        }
        if (type != ACTOR_ACTOR && type != ACTOR_ACTOR_SHOT && frame->GetType() == FRAME_MODEL)
        {
            I3D_frame *sector = frame->m_pParent;
            while (sector && sector->GetType() != FRAME_SECTOR)
                sector = sector->m_pParent;
            if (sector && sector != frame->m_pParent)
            {
                frame->LinkTo(sector, I3DLINK_UPDMATRIX);
                frame->Update();
            }
        }
        pos = frame->GetPos();
        dir = frame->GetDir();
    }
    else
    {
        pos = S_vector(0.0f, 0.0f, 0.0f);
        dir = S_vector(0.0f, 0.0f, 1.0f);
    }
    unknown_5e = false;
    unknown_5d = true;
    unknown_5f = false;
    if (type == ACTOR_ACTOR && frame)
    {
        DebustizeFrame(frame);
        NoShadowFrame(frame);
    }
}

// Puts the frame back where GameInit found it, and takes the pointer to the actor it gave the frame back. PS2's links
// the frame to its parent only where the parent is still in the scene.
// FUNCTION: GAME 0x4063e0
void C_actor::GameDone()
{
    if (!frame)
        return;
    frame->SetOn(true);
    frame->LinkTo(saved_parent, 0);
    frame->SetPos(saved_pos);
    // SetRot with a normalization, written out as C_game::DoneIntern's
    I3D_frame *frm = frame;
    frm->m_Rot = saved_rot;
    frm->m_Rot.Normalize();
    frm->m_FrmFlags = (frm->m_FrmFlags & ~(FRMFLAGS_HR_BOUND_W_VALID | FRMFLAGS_WMAT_VALID | FRMFLAGS_MAT_VALID |
        FRMFLAGS_ROT_VALID)) | FRMFLAGS_UPDATE | FRMFLAGS_QUAT_VALID;
    frame->Update();
    frame->SetOn(saved_on);
    if (frame_actor_set)
    {
        C_actor **p = GetFrameActor(frame);
        if (p && *p)
        {
            *p = NULL;
            SetFrameActor(frame, p);
        }
    }
}

#pragma auto_inline(off)

// STUB: GAME 0x4064f0
void C_actor::Tick(uint time)
{
    StubLog("C_actor::Tick");
}

// STUB: GAME 0x406770
int C_actor::SaveGameGetSize()
{
    StubLog("C_actor::SaveGameGetSize");
    return 0;
}

// STUB: GAME 0x406870
bool C_actor::SaveGameSave(void **buf)
{
    StubLog("C_actor::SaveGameSave");
    return false;
}

// STUB: GAME 0x406b00
bool C_actor::SaveGameLoad(void **buf)
{
    StubLog("C_actor::SaveGameLoad");
    return false;
}

#pragma auto_inline(on)

// From the last one, which the actor most likely is if anything
// FUNCTION: GAME 0x406da0
void C_actor::SetActState(int state)
{
    if (act_state != ACTSTATE_2 && act_state != state)
        act_mission->game->unknown_10c = true;
    act_state = state;
}

// An actor in ACTSTATE_2 goes back to ACTSTATE_0, which the game is told of as of SetActState's changes
// FUNCTION: GAME 0x406dd0
void C_actor::Rebirth()
{
    if (act_state == ACTSTATE_2)
    {
        act_state = ACTSTATE_0;
        act_mission->game->unknown_10c = true;
    }
}

// Turns the projectors off on the frame's visuals, the frame's and its children's, from the last child (PS2's order)
// FUNCTION: GAME 0x407cd0
void C_actor::DebustizeFrame(I3D_frame *frm)
{
    if (!frm)
        return;
    if (frm->GetType() == FRAME_VISUAL)
        ((I3D_visual *)frm)->SetProjectorMode(SHADOW_MODE_0);
    int i = frm->NumChildren();
    while (i--)
        DebustizeFrame(frm->GetChild(i));
}

// The same for the shadows
// FUNCTION: GAME 0x407d10
void C_actor::NoShadowFrame(I3D_frame *frm)
{
    if (!frm)
        return;
    if (frm->GetType() == FRAME_VISUAL)
        ((I3D_visual *)frm)->SetShadowMode(SHADOW_MODE_0);
    int i = frm->NumChildren();
    while (i--)
        NoShadowFrame(frm->GetChild(i));
}

// FUNCTION: GAME 0x407d50
void C_actor::AddActorToGlobal(C_actor *act)
{
    if (!act)
        return;
    for (int i = global_actors.size(); i--;)
    {
        if (global_actors[i] == act)
            return;
    }
    global_actors.push_back(act);
}

// From the last one, as AddActorToGlobal
// FUNCTION: GAME 0x407f50
void C_actor::DelActorFromGlobal(C_actor *act)
{
    for (int i = global_actors.size(); i--;)
    {
        if (global_actors[i] == act)
        {
            global_actors[i] = global_actors.back();
            global_actors.pop_back();
            return;
        }
    }
}

// From the last one too
// FUNCTION: GAME 0x407fb0
bool C_actor::IsGlobalActor(C_actor *act)
{
    if (!act)
        return false;
    for (int i = global_actors.size(); i--;)
    {
        if (global_actors[i] == act)
            return true;
    }
    return false;
}

// The frame's world position, or the actor's own without a frame (PS2 name)
// FUNCTION: GAME 0x407fe0
S_vector C_actor::GetWorldPos()
{
    if (frame)
        return frame->GetWorldPos();
    return pos;
}

#pragma auto_inline(off)

// STUB: GAME 0x408040
S_vector C_actor::GetWorldDir()
{
    StubLog("C_actor::GetWorldDir");
    return dir;
}

#pragma auto_inline(on)

// FUNCTION: GAME 0x408450
int rnd(int n)
{
    if (n < 1)
        return 0;
    return rand() % n;
}

// FUNCTION: GAME 0x408470
float rnd()
{
    return rand() * (1.0f / RAND_MAX);
}

#pragma auto_inline(off)

// STUB: GAME 0x408490
float AngleTo2(const S_vector2 &a, const S_vector2 &b)
{
    StubLog("AngleTo2");
    return 0.0f;
}

// STUB: GAME 0x408630
void RotateDir(S_vector &v, float angle)
{
    StubLog("RotateDir");
}

#pragma auto_inline(on)

// qsort's, by id
// FUNCTION: GAME 0x408690
int __cdecl compare_anim_index(const S_anim_index *a, const S_anim_index *b)
{
    uint id_a = a->id;
    uint id_b = b->id;
    if (id_a == id_b)
        return 0;
    return id_a < id_b ? -1 : 1;
}

// FUNCTION: GAME 0x4086b0
void InitAnimIndex()
{
    int i;
    for (i = 0; _anim_info_[i].id != 0xffff; i++)
        ;
    _anim_index_count_ = i;
    if (!_anim_index_count_)
        return;
    _anim_index_ = new S_anim_index[_anim_index_count_];
    for (i = 0; _anim_info_[i].id != 0xffff; i++)
    {
        _anim_index_[i].id = _anim_info_[i].id;
        _anim_index_[i].index = i;
    }
    qsort(_anim_index_, _anim_index_count_, sizeof(S_anim_index),
        (int (__cdecl *)(const void *, const void *))compare_anim_index);
}

// The pointer stays as it is, as PS2's
// FUNCTION: GAME 0x408750
void DoneAnimIndex()
{
    delete[] _anim_index_;
}

#pragma auto_inline(off)

// STUB: GAME 0x42cc20
void C_actor::vfunc_5()
{
    StubLog("C_actor::vfunc_5");
}

// An empty ret 4 of the out-of-line kind (0x403db0 is the inline one's), folded, in another file's range: C_clock's
// DuplicateFrom jumps to it
// STUB: GAME 0x4ab130
void C_actor::DuplicateFrom(C_actor *act)
{
    StubLog("C_actor::DuplicateFrom");
}

// STUB: GAME 0x5c2650
void C_actor::vfunc_11()
{
    StubLog("C_actor::vfunc_11");
}

// STUB: GAME 0x5c26a0
void C_actor::vfunc_10()
{
    StubLog("C_actor::vfunc_10");
}

// STUB: GAME 0x5c27b0
void C_actor::vfunc_6(bool on)
{
    StubLog("C_actor::vfunc_6");
}

// STUB: GAME 0x5c27d0
void C_actor::vfunc_7(bool on)
{
    StubLog("C_actor::vfunc_7");
}

// STUB: GAME 0x5c27f0
void C_actor::vfunc_8()
{
    StubLog("C_actor::vfunc_8");
}

// The vtable's 0x404990, a ret (Main.h)
void C_actor::vfunc_12()
{
    StubLog("C_actor::vfunc_12");
}

// The vtable's 0x403db0, a ret 4, the next two too (VE_BreakageGlass.h)
void C_actor::vfunc_13()
{
    StubLog("C_actor::vfunc_13");
}

void C_actor::vfunc_14()
{
    StubLog("C_actor::vfunc_14");
}

void C_actor::vfunc_15()
{
    StubLog("C_actor::vfunc_15");
}

// The vtable's 0x4aa490, which returns true (a mov al, 1 and a ret 4), as LoadData's
void C_actor::SaveData(C_chunk *chunk)
{
    StubLog("C_actor::SaveData");
}

void C_actor::LoadData(C_chunk *chunk)
{
    StubLog("C_actor::LoadData");
}

#pragma auto_inline(on)
