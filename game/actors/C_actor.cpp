// The actors (PS2 C_actor.cpp; the PC's file is the one at 0x404ce0-0x40938c, docs/game-files.md), written whole: the
// animations' table and their index, which InitSystem builds and CloseSystem frees, the cutscenes' recording and
// playing of an actor (PC only, C_actor's first slots, with C_actor_shot's two whose copies lie here), the global
// actors' list, the constructor, Init, GameInit and GameDone, Tick, the saved games, the possible collisions with the
// frames of posscoll_frame, the 2D bounds, the world direction and the free helpers (rnd, AngleTo2, RotateDir). The
// out-of-line copies after UnlinkAllFrameChild are the headers' (the model cache's Open, I3D_cache.h, and its helpers;
// Release and the class's constant functions, C_actor.h). Open: IsPossibleCollAll (see its comment), whose S_vector
// copies the original compiled before it and ours at the file's end (/YX), the model cache's Open and the order of the
// copies after UnlinkAllFrameChild. GetRecordRot, whose copy lies in C_car.cpp's range, is at the end; C_actor's
// records functions are in C_cutscene_records.cpp.

#include "actors/C_actor.h"

#include <float.h>
#include <math.h>
#include <stdlib.h>

#include "actors/C_actor_shot.h"
#include "ai/C_person_traffic.h"
#include "ai/road_web.h"

#include "I3D_cache.h"

#include "mission/Mission.h"
#include "mission/game.h"
#include "mission/C_recorder.h"
#include "mission/C_effects.h"

// The file's only initialized data, and its literals come before its functions'. An id is the animation in its low nine
// bits with at most one bit above them for its set: the eight sets of breathing, running and walking, the corner
// animations' weapons and stances.
// GLOBAL: GAME 0x62e488
S_anim_info _anim_info_[] = {
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

// Adds the actor to the cutscene, with the size of its records (one kind)
// FUNCTION: GAME 0x404e00
bool C_actor::AddToCutscene()
{
    cutscene.AddActor(this, sizeof(S_actor_record), 0, 0);
    return true;
}

// C_actor_shot's (its copies are among C_actor.cpp's)
// FUNCTION: GAME 0x404e20
bool C_actor_shot::AddToCutscene()
{
    cutscene.AddActor(this, sizeof(S_actor_shot_record), 0, 0);
    return true;
}

// Plays the current record: a particle started at the frame or stopped, or else the weapon and the shooting set
// FUNCTION: GAME 0x404e40
void C_actor_shot::PlayRecord(int time)
{
    S_actor_shot_record *rec = (S_actor_shot_record *)records->cur;
    if (rec->flags & (SHOT_REC_PARTICLE_START | SHOT_REC_PARTICLE_STOP))
    {
        if (rec->flags & SHOT_REC_PARTICLE_START)
        {
            if (particle >= 0)
                act_mission->effects->DestroyParticle(particle);
            particle = act_mission->GetEffects()->CreateParticle(rec->id, S_vector(0.0f, 0.0f, 0.0f),
                S_vector(0.0f, 0.0f, 1.0f), 0, frame);
        }
        else
        {
            if (particle >= 0)
                act_mission->effects->DestroyParticle(particle);
            particle = -1;
        }
        return;
    }
    if (rec->flags & SHOT_REC_WEAPON)
        SetWeapon(rec->id);
    bool s = (rec->flags & SHOT_REC_SHOOTS) != 0;
    bool f = (rec->flags & SHOT_REC_FIRING) != 0;
    shot_time = -1;
    firing = f;
    shoots = s;
}

// FUNCTION: GAME 0x404f30
int C_actor::RecordKey(int time)
{
    return -1;
}

// Plays the current record's animation on a model, from the time it was at when the record was made
// FUNCTION: GAME 0x404f40
void C_actor::PlayRecord(int time)
{
    S_actor_record *rec = (S_actor_record *)records->cur;
    uint flags = rec->anim;
    if (!(flags & ACTOR_REC_ANIMATED))
        return;
    I3D_model *model = (I3D_model *)frame;
    if (model->GetType() != FRAME_MODEL)
        return;
    S_cutscene_anim *a = cutscene.GetAnimByIndex(flags & ACTOR_REC_ANIM);
    if (a)
    {
        uint start = (rec->anim >> ACTOR_REC_ANIM_TIME_SHIFT) * 40;
        uint t;
        if (flags & ACTOR_REC_LOOP)
        {
            t = (start - rec->time + time) % a->anim_set->GetEndTime();
            a->anim_set->SetOptions(I3DANIMOP_LOOP);
        }
        else
        {
            t = start - rec->time + time;
            a->anim_set->SetOptions(I3DANIMOP_ONCE);
        }
        model->SetAnimation(a->anim_set, 0, 1.0f, 0);
        model->SetAnimTime(0, t);
    }
    model->Tick(0);
    model->Update();
}

// Plays the records for a step of the time: the frame between the current record and the next by the cutscene's time,
// on the spline (Ease's ease, Hermite's basis functions) and slerped, a model ticked, the frame's sector set where its
// parent is the scene's, and the scene's camera put where the frame is for a camera's record
// FUNCTION: GAME 0x405010
void C_actor::PlayStep(int time)
{
    S_actor_record *rec = (S_actor_record *)records->cur;
    S_actor_record *next = (S_actor_record *)records->next;
    if (cutscene.scaled_time > rec->time && !(rec->anim & ACTOR_REC_NO_BLEND))
    {
        if (next != rec && cutscene.scaled_time <= next->time)
        {
            float f = (float)(cutscene.scaled_time - rec->time) / (next->time - rec->time);
            float t = Ease(f, rec->rot_ease_from, next->rot_ease_to);
            float s = Ease(f, rec->ease_from, next->ease_to);
            float s2 = s * s;
            float s3 = s2 * s;
            S_vector pos;
            pos = rec->pos * (2.0f * s3 - 3.0f * s2 + 1.0f);
            pos += next->pos * (3.0f * s2 - 2.0f * s3);
            pos += rec->tangent_out * (s3 - 2.0f * s2 + s);
            pos += next->tangent_in * (s3 - s2);
            frame->SetPos(pos);
            S_quat rot = rec->rot;
            rot = rot.Slerp(next->rot, t, true);
            frame->SetRot(rot);
        }
        else
        {
            frame->SetPos(next->pos);
            frame->SetRot(next->rot);
        }
    }
    else
    {
        frame->SetPos(rec->pos);
        frame->SetRot(rec->rot);
    }
    if (frame->GetType() == FRAME_MODEL)
        frame->Tick(time);
    frame->Update();
    if (!frame->GetParent() || frame->GetParent()->GetType() == FRAME_SECTOR)
        act_mission->scene->SetFrameSector(frame);
    if (rec->anim & ACTOR_REC_CAMERA)
    {
        I3D_camera *camera = act_mission->scene->GetActiveCamera();
        camera->SetPos(frame->GetWorldPos());
        camera->SetRot(frame->GetWorldRot());
        camera->Update();
    }
}

// Places a plain actor (ACTOR_ACTOR) as the records give it for the time (a jump in the cutscene): the frame between
// the last record up to the time and the next, as PlayStep does, the record's animation as PlayRecord does, and for a
// camera's record the frame's world placement kept for the cutscene's camera
// FUNCTION: GAME 0x4054c0
bool C_actor::PlaceAtTime(int time)
{
    if (!records || type != ACTOR_ACTOR || !records->first_block)
        return true;
    S_actor_record *rec = (S_actor_record *)(records->first_block + 8);
    S_actor_record *prev = rec;
    while (rec && rec->time <= records->last_key_time)
    {
        if (rec->time <= time)
            prev = rec;
        if (rec->time >= records->last_key_time)
            break;
        rec = (S_actor_record *)((uchar *)rec + records->record_sizes[rec->type]);
    }
    S_actor_record *next = prev->time < records->last_key_time ?
        (S_actor_record *)((uchar *)prev + records->record_sizes[prev->type]) : prev;
    if (time > prev->time && !(prev->anim & ACTOR_REC_NO_BLEND))
    {
        if (next != prev && time <= next->time)
        {
            float f = (float)(time - prev->time) / (next->time - prev->time);
            float t = Ease(f, prev->rot_ease_from, next->rot_ease_to);
            float s = Ease(f, prev->ease_from, next->ease_to);
            float s2 = s * s;
            float s3 = s2 * s;
            S_vector pos;
            pos = prev->pos * (2.0f * s3 - 3.0f * s2 + 1.0f);
            pos += next->pos * (3.0f * s2 - 2.0f * s3);
            pos += prev->tangent_out * (s3 - 2.0f * s2 + s);
            pos += next->tangent_in * (s3 - s2);
            frame->SetPos(pos);
            S_quat rot = prev->rot;
            rot = rot.Slerp(next->rot, t, true);
            frame->SetRot(rot);
        }
        else
        {
            frame->SetPos(next->pos);
            frame->SetRot(next->rot);
        }
    }
    else
    {
        frame->SetPos(prev->pos);
        frame->SetRot(prev->rot);
    }
    uint flags = prev->anim;
    if ((flags & ACTOR_REC_ANIMATED) && frame->GetType() == FRAME_MODEL)
    {
        I3D_model *model = (I3D_model *)frame;
        S_cutscene_anim *a = cutscene.GetAnimByIndex(flags & ACTOR_REC_ANIM);
        if (a)
        {
            uint start = (prev->anim >> ACTOR_REC_ANIM_TIME_SHIFT) * 40;
            uint t;
            if (flags & ACTOR_REC_LOOP)
            {
                t = (start - prev->time + time) % a->anim_set->GetEndTime();
                a->anim_set->SetOptions(I3DANIMOP_LOOP);
            }
            else
            {
                t = start - prev->time + time;
                a->anim_set->SetOptions(I3DANIMOP_ONCE);
            }
            model->SetAnimation(a->anim_set, 0, 1.0f, 0);
            model->SetAnimTime(0, t);
        }
        model->Tick(0);
        model->Update();
    }
    frame->Update();
    if (!frame->GetParent() || frame->GetParent()->GetType() == FRAME_SECTOR)
        act_mission->scene->SetFrameSector(frame);
    if (prev->anim & ACTOR_REC_CAMERA)
    {
        cutscene.camera_placed = true;
        cutscene.camera_pos = frame->GetWorldPos();
        cutscene.camera_rot = frame->GetWorldRot();
    }
    return true;
}

// The position the records give for the time, as PlaceAtTime places the frame (0 without records)
// FUNCTION: GAME 0x405a00
S_vector C_actor::GetRecordPos(int time)
{
    if (!records->first_block)
        return S_vector(0.0f, 0.0f, 0.0f);
    S_actor_record *rec = (S_actor_record *)(records->first_block + 8);
    S_actor_record *prev = rec;
    while (rec && rec->time <= records->last_key_time)
    {
        if (rec->time <= time)
            prev = rec;
        if (rec->time >= records->last_key_time)
            break;
        rec = (S_actor_record *)((uchar *)rec + records->record_sizes[rec->type]);
    }
    S_actor_record *next = prev->time < records->last_key_time ?
        (S_actor_record *)((uchar *)prev + records->record_sizes[prev->type]) : prev;
    if (time <= prev->time)
        return prev->pos;
    if (next != prev && time <= next->time)
    {
        float f = (float)(time - prev->time) / (next->time - prev->time);
        // the rotation's ease too, not used
        float t = Ease(f, prev->rot_ease_from, next->rot_ease_to);
        float s = Ease(f, prev->ease_from, next->ease_to);
        float s2 = s * s;
        float s3 = s2 * s;
        S_vector pos;
        pos = prev->pos * (2.0f * s3 - 3.0f * s2 + 1.0f);
        pos += next->pos * (3.0f * s2 - 2.0f * s3);
        pos += prev->tangent_out * (s3 - 2.0f * s2 + s);
        pos += next->tangent_in * (s3 - s2);
        return pos;
    }
    return next->pos;
}

// Their initializers come before the constructor, global_actors' (0x405c20) first, then posscoll_frame's (0x405ca0).
// Their destructors (0x405c60, 0x405ce0) test and set a guard byte they share (0x63d2b4), a bit each: VC6's code for
// these definitions, ours the same. Open: reccmp scores the initializers' first halves 50%, as nothing names the
// atexit registrations they jump to (0x405c50, 0x405cd0; ours $E numbers).
// GLOBAL: GAME 0x63d2b8
std::vector<C_actor *> C_actor::global_actors;

// GLOBAL: GAME 0x63d2a4
std::vector<C_actor::S_posscoll_frame> C_actor::posscoll_frame;

// FUNCTION: GAME 0x405d20
C_actor::C_actor() :
    act_flags(0),
    ref(1),
    type(ACTOR_ACTOR),
    subtype(1),
    net_direct(false),
    cutscene_link(0),
    from_mission(false),
    frame(NULL)
{
    records = 0;
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

// Empty: the vtable's entry is the linker's folded empty ret 4 of the out-of-line kind (0x4ab130, C_actor.h; 0x403db0
// is the inline one's), which C_clock's DuplicateFrom jumps to
void C_actor::DuplicateFrom(C_actor *act)
{
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
    number = ++mission->last_actor_number;
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
    act_state = ACTSTATE_ACTIVE;
    return true;
}

// GameInit's saving of the frame's placement, but for whether it is on (C_cartridge::GameDone has it written out);
// called from 0x5bd910 (Main.cpp's range, not written yet)
// FUNCTION: GAME 0x405e60
void C_actor::InternalPlacementReset()
{
    if (!frame)
        return;
    saved_pos = frame->GetPos();
    saved_rot = frame->GetRot();
    saved_parent = frame->m_pParent;
}

// GameDone's placement, without its frame turned on first and its pointer to the actor kept
// FUNCTION: GAME 0x405ee0
void C_actor::ResetPlacement()
{
    if (!frame)
        return;
    frame->LinkTo(saved_parent, 0);
    frame->SetPos(saved_pos);
    // SetRot with a normalization, written out as GameDone's
    I3D_frame *frm = frame;
    frm->m_Rot = saved_rot;
    frm->m_Rot.Normalize();
    frm->m_FrmFlags = (frm->m_FrmFlags & ~(FRMFLAGS_HR_BOUND_W_VALID | FRMFLAGS_WMAT_VALID | FRMFLAGS_MAT_VALID |
        FRMFLAGS_ROT_VALID)) | FRMFLAGS_UPDATE | FRMFLAGS_QUAT_VALID;
    frame->Update();
    frame->SetOn(saved_on);
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
    act_state = ACTSTATE_ACTIVE;
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
    dead = false;
    alive = true;
    rendered = false;
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

// Whether the frame was rendered in the scene's last frame, for a model or a visual (the same test twice, as PS2's and
// C_game::Tick's), then the actor's AI and update, and its death once, the first tick that finds it neither alive nor
// dead
// FUNCTION: GAME 0x4064f0
void C_actor::Tick(uint time)
{
    if (frame->GetType() == FRAME_MODEL)
        rendered = frame->m_RenderNumber == act_mission->GetScene()->m_RenderNumber;
    else if (frame->GetType() == FRAME_VISUAL)
        rendered = frame->m_RenderNumber == act_mission->GetScene()->m_RenderNumber;
    else
        rendered = true;
    if (!(act_flags & (ACTF_CUTSCENE_PLAYED | ACTF_CUTSCENE_PLAY)))
    {
        if (net_direct)
            NetDirect(time);
        else
            AI(time);
    }
    Update(time);
    if (!alive && !dead)
    {
        dead = true;
        alive = false;
        Death();
    }
}

// A string of a saved game, a word of its length and its characters (the names are ours): SaveGameSave writes the
// models' three this way and SaveGameLoad reads the last two, the same code in each copy. The length is taken before
// the test for NULL, as in the original (PS2's the same).
inline void SaveGameSaveString(void **buf, const char *s)
{
    ushort len = strlen(s);
    *(ushort *)*buf = len;
    *buf = (uchar *)*buf + sizeof(ushort);
    if (s)
    {
        memcpy(*buf, s, len);
        *buf = (uchar *)*buf + len;
    }
}

// Reads one into s, ended with a zero
inline void SaveGameLoadString(void **buf, char *s)
{
    ushort len = *(ushort *)*buf;
    *buf = (uchar *)*buf + sizeof(ushort);
    memcpy(s, *buf, len);
    s[len] = 0;
    *buf = (uchar *)*buf + len;
}

// An actor in a saved game (the human's cars): the length of its frame's name with its end and the actor's type, then
// the name; two zeros for none, or for an actor whose frame has no name. PS2's rounds each size up to 4 bytes
// (ZatocToNa4, a static function of PS2's file), the PC's doesn't.
// FUNCTION: GAME 0x406590
int C_actor::Actor_SaveGameGetSize(C_actor *act)
{
    if (!act || !act->frame || !strlen(act->frame->GetName()))
        return sizeof(int) * 2;
    return sizeof(int) * 2 + strlen(act->frame->GetName()) + 1;
}

// FUNCTION: GAME 0x4065f0
bool C_actor::Actor_SaveGameSave(void **buf, C_actor *act)
{
    if (!act || !act->frame || !strlen(act->frame->GetName()))
    {
        int *p = (int *)*buf;
        *buf = p + 2;
        p[0] = 0;
        p[1] = 0;
        return true;
    }
    int len = strlen(act->frame->GetName()) + 1;
    int *p = (int *)*buf;
    *p++ = len;
    *p++ = act->type;
    memcpy(p, act->frame->GetName(), len);
    *buf = (uchar *)*buf + sizeof(int) * 2 + len;
    return true;
}

// The first of global_actors whose frame has the name and that has the type, NULL where none has. act is set before
// the pointer is moved on, the order that gives its zero the loop counter's register from the start (PS2 sets it after
// the buffer).
// FUNCTION: GAME 0x4066a0
C_actor *C_actor::Actor_SaveGameLoad(void **buf)
{
    int *p = (int *)*buf;
    int len = *p;
    if (!len)
    {
        *buf = (uchar *)*buf + sizeof(int) * 2;
        return NULL;
    }
    C_actor *act = NULL;
    p++;
    int type = *p++;
    *buf = (uchar *)*buf + sizeof(int) * 2 + len;
    for (uint i = 0; i < global_actors.size(); i++)
    {
        if (global_actors[i]->frame && !strcmp(global_actors[i]->frame->GetName(), (const char *)p) &&
            global_actors[i]->type == type)
        {
            act = global_actors[i];
            break;
        }
    }
    return act;
}

// The struct, and for a car the models the script put into its model (SETMODELTOCAR), from its frames' last: each
// one's file's name without the folder, its name and its parent's, each a word of its length and the characters, then
// its scale, position and rotation. PS2's rounds each size up to 4 bytes (ZatocToNa4), and its parts are 2 bytes
// longer.
// FUNCTION: GAME 0x406770
int C_actor::SaveGameGetSize()
{
    int size = sizeof(S_actor_savegame);
    if (frame && type == ACTOR_CAR)
    {
        I3D_model *model = (I3D_model *)frame;
        int i = model->frames.size();
        I3D_frame **frames = model->frames.array;
        while (i--)
        {
            if (frames[i]->GetType() != FRAME_MODEL)
                continue;
            char name[256];
            strcpy(name, ((I3D_model *)frames[i])->GetFileName());
            char *s = strrchr(name, '\\');
            if (s)
                s++;
            size += sizeof(ushort);
            if (s)
                size += strlen(s);
            size += sizeof(ushort);
            size += strlen(frames[i]->GetName());
            size += sizeof(ushort);
            if (frames[i]->GetParent())
                size += strlen(frames[i]->GetParent()->GetName());
            size += sizeof(S_vector) * 2 + sizeof(S_quat);
        }
    }
    return size;
}

// A file's name without a backslash would be saved through NULL (strlen of it), as PS2's (whose strrchr takes the
// whole name then); PS2 also saves a player's frame as on
// FUNCTION: GAME 0x406870
bool C_actor::SaveGameSave(void **buf)
{
    S_actor_savegame *s = (S_actor_savegame *)*buf;
    s->version = ACTOR_SAVEGAME_VERSION;
    s->act_state = act_state;
    s->number = number;
    s->dead = dead;
    s->alive = alive;
    s->num_models = 0;
    if (frame)
        s->frame_on = frame->IsOn();
    else
        s->frame_on = false;
    *buf = (uchar *)*buf + sizeof(S_actor_savegame);
    if (frame && type == ACTOR_CAR)
    {
        I3D_model *model = (I3D_model *)frame;
        int i = model->frames.size();
        I3D_frame **frames = model->frames.array;
        while (i--)
        {
            if (frames[i]->GetType() != FRAME_MODEL)
                continue;
            s->num_models++;
            char name[256];
            strcpy(name, ((I3D_model *)frames[i])->GetFileName());
            char *p = strrchr(name, '\\');
            if (p)
                p++;
            SaveGameSaveString(buf, p);
            SaveGameSaveString(buf, frames[i]->GetName());
            if (frames[i]->GetParent())
                SaveGameSaveString(buf, frames[i]->GetParent()->GetName());
            else
            {
                *(ushort *)*buf = 0;
                *buf = (uchar *)*buf + sizeof(ushort);
            }
            *(S_vector *)*buf = frames[i]->GetScale();
            *buf = (uchar *)*buf + sizeof(S_vector);
            *(S_vector *)*buf = frames[i]->GetPos();
            *buf = (uchar *)*buf + sizeof(S_vector);
            *(S_quat *)*buf = frames[i]->GetRot();
            *buf = (uchar *)*buf + sizeof(S_quat);
        }
    }
    return true;
}

// Each model is created again, added to the actor's model and loaded through the model cache, then named, linked to
// the scene's frame of its parent's name and placed. A model saved without a file's name is skipped after its word
// only, the rest of its data left in the buffer (PS2's the same). The state is read first and set after the frame
// (PS2's order); PS2's inlines the cache's Open.
// FUNCTION: GAME 0x406b00
bool C_actor::SaveGameLoad(void **buf)
{
    S_actor_savegame *s = (S_actor_savegame *)*buf;
    if (s->version != ACTOR_SAVEGAME_VERSION)
        return false;
    int state = s->act_state;
    number = s->number;
    dead = s->dead;
    alive = s->alive;
    if (frame)
        frame->SetOn(s->frame_on);
    SetActState(state);
    *buf = (uchar *)*buf + sizeof(S_actor_savegame);
    for (int i = s->num_models; i; i--)
    {
        char name[256];
        ushort len = *(ushort *)*buf;
        *buf = (uchar *)*buf + sizeof(ushort);
        if (len < 1)
            continue;
        memcpy(name, *buf, len);
        name[len] = 0;
        *buf = (uchar *)*buf + len;
        I3D_model *model = (I3D_model *)driver->CreateFrame(FRAME_MODEL);
        ((I3D_model *)frame)->AddFrame(model);
        model->Release();
        model_cache.Open(model, name, 0, NULL, NULL, NULL);
        SaveGameLoadString(buf, name);
        model->SetName(name);
        SaveGameLoadString(buf, name);
        I3D_frame *parent = act_mission->GetScene()->FindFrame(name, ENUMF_ALL);
        model->LinkTo(parent, 0);
        model->SetScale(*(S_vector *)*buf);
        *buf = (uchar *)*buf + sizeof(S_vector);
        model->SetPos(*(S_vector *)*buf);
        *buf = (uchar *)*buf + sizeof(S_vector);
        model->SetRot(*(S_quat *)*buf);
        *buf = (uchar *)*buf + sizeof(S_quat);
        model->Update();
    }
    return true;
}

// The buffer stays where it is
// FUNCTION: GAME 0x406d80
void C_actor::SaveGameLoadGuid(void **buf)
{
    S_actor_savegame *s = (S_actor_savegame *)*buf;
    if (s->version == ACTOR_SAVEGAME_VERSION)
        number = s->number;
}

// Empty, as PS2's: the vtable's entry is the linker's folded empty function (0x4aabe0, input/C_ff_manager.h)
void C_actor::Death()
{
}

// An actor in ACTSTATE_NONE stays in it (only Rebirth takes it back), as PS2's
// FUNCTION: GAME 0x406da0
void C_actor::SetActState(int state)
{
    if (act_state == ACTSTATE_NONE || act_state == state)
        return;
    act_mission->game->act_state_changed = true;
    act_state = state;
}

// An actor in ACTSTATE_NONE goes back to ACTSTATE_ACTIVE, which the game is told of as of SetActState's changes
// FUNCTION: GAME 0x406dd0
void C_actor::Rebirth()
{
    if (act_state == ACTSTATE_NONE)
    {
        act_state = ACTSTATE_ACTIVE;
        act_mission->game->act_state_changed = true;
    }
}

// The part of the move from pos by dir after which a sphere of radius + add around center is first met, 0 where it is
// met already, negative where it isn't: C_human::IsPossibleColl's test, the sphere not moving (the name is ours; PS2
// inlines it too). An inline function: its two copies in IsPossibleCollAll inline different calls (the frames' more),
// as nested calls get shares of the budget, where code written out twice inlines every call. The arguments are
// evaluated right to left: center is copied before the frame's radius is read, which comes after the constructor's
// calls. move is a local: its zero stores come before the road block's center is copied, where a temporary in
// move - dir has them after d's constructor call.
static inline float PossibleCollSphere(float add, S_vector center, const S_vector &pos, const S_vector &dir,
    float radius)
{
    S_vector move(0.0f, 0.0f, 0.0f);
    S_vector d = center - pos;
    S_vector v = move - dir;
    float r = radius + add;
    float rr = r * r;
    float t;
    if (d.Dot(d) <= rr)
        t = 0.0f;
    else
    {
        float a = v.Dot(v);
        if (a <= 0.0f)
            t = -1.0f;
        else
        {
            float b = v.Dot(d) * 2.0f;
            float disc = b * b - (d.Dot(d) - rr) * a * 4.0f;
            if (disc < 0.0f)
                t = disc;
            else
            {
                float s = (float)sqrt(disc);
                float inv = 1.0f / (a * 2.0f);
                float t1 = (s - b) * inv;
                t = (-b - s) * inv;
                if (t1 < t)
                    t = t1;
            }
        }
    }
    return t;
}

// Whether the actor at pos, moving by dir, may run into something within the move: the active actors, the inactive
// cars and the don's tail (each one's IsPossibleColl, its part of the move at most 1), and while the police are
// active their road blocks that are up (a sphere of radius + 40 around each one's node) and the frames of
// posscoll_frame (a sphere of radius + the frame's), each list from the last one. out is the nearest such place, coll
// the actor there (the don's tail for the road blocks and the frames, as PS2's). The vectors by value, PS2's by
// reference. Open: the road blocks' loop holds the block in eax where the original has ecx, and loads the mission's
// roads before the node; the frames' loop reads the radius after the S_vector constructor's call only where VC6 has
// compiled the constructor's copy before this function, which /YX does in the original (its S_vector copies come right
// after it, 0x407540) and at the file's end in ours.
// FUNCTION: GAME 0x406df0
bool C_actor::IsPossibleCollAll(S_vector pos, S_vector dir, float radius, S_vector &out, C_actor **coll)
{
    float best = FLT_MAX;
    bool ret = false;
    if (coll)
        *coll = NULL;
    std::vector<C_actor *> &active = act_mission->game->active_actors;
    C_actor *act;
    int i;
    for (i = active.size(); i--;)
    {
        if (active[i] == this)
            continue;
        act = NULL;
        float dist;
        bool hit = active[i]->IsPossibleColl(this, pos, dir, radius, dist, &act);
        if (hit && dist > 1.0f)
            hit = false;
        ret |= hit;
        if (hit && dist < best)
        {
            best = dist;
            if (coll)
                *coll = act;
        }
    }
    std::vector<C_actor *> &inactive = act_mission->game->inactive_actors;
    for (i = inactive.size(); i--;)
    {
        if (inactive[i]->type == ACTOR_CAR && inactive[i] != this)
        {
            act = NULL;
            float dist;
            bool hit = inactive[i]->IsPossibleColl(this, pos, dir, radius, dist, &act);
            if (hit && dist > 1.0f)
                hit = false;
            ret |= hit;
            if (hit && dist < best)
            {
                best = dist;
                if (coll)
                    *coll = act;
            }
        }
    }
    {
        act = NULL;
        float dist;
        bool hit = act_mission->game->don_ocas->IsPossibleColl(this, pos, dir, radius, dist, &act);
        if (hit && dist > 1.0f)
            hit = false;
        ret |= hit;
        if (hit && dist < best)
        {
            best = dist;
            if (coll)
                *coll = act_mission->game->don_ocas;
        }
    }
    C_game *game = act_mission->game;
    if (game->shvestky.active)
    {
        for (i = game->road_blocks.size(); i--;)
        {
            C_game::S_RoadBlock &rb = game->road_blocks[i];
            if (!rb.active)
                continue;
            float t = PossibleCollSphere(40.0f, act_mission->roads->links[rb.node].pos, pos, dir, radius);
            if (t < best && t >= 0.0f)
            {
                best = t;
                ret = true;
                if (coll)
                    *coll = act_mission->game->don_ocas;
            }
        }
    }
    for (i = posscoll_frame.size(); i--;)
    {
        float t = PossibleCollSphere(posscoll_frame[i].radius, posscoll_frame[i].frame->GetWorldPos(), pos, dir,
            radius);
        if (t < best && t >= 0.0f)
        {
            best = t;
            ret = true;
            if (coll)
                *coll = act_mission->game->don_ocas;
        }
    }
    if (ret)
        out = pos + dir * best;
    return ret;
}

// The frame's outline seen from above, as world points in x and z (PS2 name; frm unused, as PS2's), false without a
// frame. An upright frame (its y axis within 45 degrees of the world's) gives the four corners of its hierarchy's box
// at half its height, 0.2 out on each side; any other the four of the box's eight corners that lie furthest in +x, -x,
// +z and -z, every coordinate 0.2 lower (the maximum's too: PS2's the same). PS2's CompletionBoudVolume, after
// RotateDirNoNorm there, has no PC copy and no part in it. The centre goes through S_vector's operators though only its
// y is used: written (min.y + max.y) * .5f, VC6 inlines more of the last push_back (its size() and the second _Ucopy)
// than the original. The upright points go through a local assigned from a temporary, the others straight into
// push_back (PS2 copies the same ones twice). Open: the original schedules the matrix copy's setup before the centre's
// product, ours after it.
// FUNCTION: GAME 0x4075a0
bool C_actor::GetBoundPoints2D(std::vector<S_vector2> &points, I3D_frame *frm)
{
    if (!frame)
        return false;
    if (frame->GetMatrix()(1).Dot(S_vector(0.0f, 1.0f, 0.0f)) > .7071f)
    {
        I3D_bvolume bvol = frame->GetHRBoundVolume();
        S_matrix m = frame->GetMatrix();
        S_vector center = (bvol.bbox.min + bvol.bbox.max) * .5f;
        float y = center.y;
        S_vector corners[4];
        corners[0] = S_vector(bvol.bbox.min.x - .2f, y, bvol.bbox.min.z - .2f);
        corners[1] = S_vector(bvol.bbox.max.x + .2f, y, bvol.bbox.min.z - .2f);
        corners[2] = S_vector(bvol.bbox.max.x + .2f, y, bvol.bbox.max.z + .2f);
        corners[3] = S_vector(bvol.bbox.min.x - .2f, y, bvol.bbox.max.z + .2f);
        S_vector2 p;
        for (int i = 0; i < 4; i++)
        {
            corners[i] = corners[i] * m;
            p = S_vector2(corners[i].x, corners[i].z);
            points.push_back(p);
        }
    }
    else
    {
        I3D_bvolume bvol = frame->GetHRBoundVolume();
        S_matrix m = frame->GetMatrix();
        S_vector c[8];
        c[0] = S_vector(bvol.bbox.min.x - .2f, bvol.bbox.min.y - .2f, bvol.bbox.min.z - .2f) * m;
        c[1] = S_vector(bvol.bbox.min.x - .2f, bvol.bbox.max.y - .2f, bvol.bbox.min.z - .2f) * m;
        c[2] = S_vector(bvol.bbox.min.x - .2f, bvol.bbox.min.y - .2f, bvol.bbox.max.z - .2f) * m;
        c[3] = S_vector(bvol.bbox.min.x - .2f, bvol.bbox.max.y - .2f, bvol.bbox.max.z - .2f) * m;
        c[4] = S_vector(bvol.bbox.max.x - .2f, bvol.bbox.min.y - .2f, bvol.bbox.min.z - .2f) * m;
        c[5] = S_vector(bvol.bbox.max.x - .2f, bvol.bbox.max.y - .2f, bvol.bbox.min.z - .2f) * m;
        c[6] = S_vector(bvol.bbox.max.x - .2f, bvol.bbox.min.y - .2f, bvol.bbox.max.z - .2f) * m;
        c[7] = S_vector(bvol.bbox.max.x - .2f, bvol.bbox.max.y - .2f, bvol.bbox.max.z - .2f) * m;
        // The corners furthest in +x, -x, +z and -z, starting from the last one
        int idx[4];
        int i = 4;
        while (i--)
            idx[i] = 7;
        float xs[2], zs[2];
        i = 2;
        while (i--)
        {
            xs[i] = c[7].x;
            zs[i] = c[7].z;
        }
        i = 7;
        while (i--)
        {
            if (c[i].x > xs[0])
            {
                idx[0] = i;
                xs[0] = c[i].x;
            }
            if (c[i].x < xs[1])
            {
                idx[1] = i;
                xs[1] = c[i].x;
            }
            if (c[i].z > zs[0])
            {
                idx[2] = i;
                zs[0] = c[i].z;
            }
            if (c[i].z < zs[1])
            {
                idx[3] = i;
                zs[1] = c[i].z;
            }
        }
        i = 4;
        while (i--)
            points.push_back(S_vector2(c[idx[i]].x, c[idx[i]].z));
    }
    return true;
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

// The same for the direction, normalized (PS2 name)
// FUNCTION: GAME 0x408040
S_vector C_actor::GetWorldDir()
{
    if (frame)
        return frame->GetWorldDir();
    return dir;
}

// The frame's radius replaced where it is there already (PS2 name)
// FUNCTION: GAME 0x4081d0
void C_actor::AddFrameForPossibleColl(I3D_frame *frm, float radius)
{
    if (!frm)
        return;
    for (int i = posscoll_frame.size(); i--;)
    {
        if (posscoll_frame[i].frame == frm)
        {
            posscoll_frame[i].radius = radius;
            return;
        }
    }
    S_posscoll_frame pf;
    pf.frame = frm;
    pf.radius = radius;
    posscoll_frame.push_back(pf);
}

// The last one in its place, as DelActorFromGlobal (PS2 name)
// FUNCTION: GAME 0x4083e0
void C_actor::DelFrameForPossibleColl(I3D_frame *frm)
{
    for (int i = posscoll_frame.size(); i--;)
    {
        if (posscoll_frame[i].frame == frm)
        {
            posscoll_frame[i] = posscoll_frame.back();
            posscoll_frame.pop_back();
            return;
        }
    }
}

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

// The difference of the two directions' atan2s, brought into -pi to pi (PS2 name)
// FUNCTION: GAME 0x408490
float AngleTo2(const S_vector2 &a, const S_vector2 &b)
{
    float aa = (float)atan2(a.y, a.x);
    float ab = (float)atan2(b.y, b.x);
    float angle = ab - aa;
    if (angle < -PI)
        angle += PI * 2.0f;
    if (angle > PI)
        angle -= PI * 2.0f;
    return angle;
}

// The rotation about y as C_human::Movement writes it, but for the sine taken into a local first (computed before the
// cosine, where PS2 calls cosf first). RotateDir keeps the vector horizontal: its y taken out before the normalization.
// FUNCTION: GAME 0x4084d0
void RotateDir(S_vector &v, float angle)
{
    S_matrix m;
    m.Identity();
    float s = (float)sin(angle);
    float c = (float)cos(angle);
    m(0, 0) = m(2, 2) = c;
    m(2, 0) = -(m(0, 2) = s);
    v = v.RotateByMatrix(m);
    v.y = 0.0f;
    v.Normalize();
}

// FUNCTION: GAME 0x408630
void RotateDirNoNorm(S_vector &v, float angle)
{
    S_matrix m;
    m.Identity();
    float s = (float)sin(angle);
    float c = (float)cos(angle);
    m(0, 0) = m(2, 2) = c;
    m(2, 0) = -(m(0, 2) = s);
    v = v.RotateByMatrix(m);
}

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
    for (i = 0; _anim_info_[i].id != ANIM_INFO_END; i++)
        ;
    _anim_index_count_ = i;
    if (!_anim_index_count_)
        return;
    _anim_index_ = new S_anim_index[_anim_index_count_];
    for (i = 0; _anim_info_[i].id != ANIM_INFO_END; i++)
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

// FUNCTION: GAME 0x408760
int GetAnimIndex(uint id)
{
    if (!_anim_index_)
        return -1;
    int lo = 0;
    int hi = _anim_index_count_ - 1;
    while (true)
    {
        int mid = (lo + hi) / 2;
        if (_anim_index_[mid].id == id)
            return _anim_index_[mid].index;
        if (lo >= hi)
            return -1;
        if (_anim_index_[mid].id > id)
            hi = mid - 1;
        else
            lo = mid + 1;
    }
}

// Each child linked to nothing, the next one taken first (PS2's the same)
// FUNCTION: GAME 0x4087b0
void UnlinkAllFrameChild(I3D_frame *frm)
{
    I3D_frame *child = frm->GetChild(0);
    while (child)
    {
        I3D_frame *f = child;
        child = child->m_pNext;
        f->LinkTo(NULL, 0);
    }
}

// Its copy lies in C_car.cpp's range: where it was defined is open (C_actor's other cutscene functions there are
// C_cutscene_records.cpp's)

// The rotation the records give for the time: slerped between the keys without their ease (0 without records)
// FUNCTION: GAME 0x42cc20
S_quat C_actor::GetRecordRot(int time)
{
    if (!records->first_block)
        return S_quat(0.0f, 0.0f, 0.0f, 0.0f);
    S_actor_record *rec = (S_actor_record *)(records->first_block + 8);
    S_actor_record *prev = rec;
    while (rec && rec->time <= records->last_key_time)
    {
        if (rec->time <= time)
            prev = rec;
        if (rec->time >= records->last_key_time)
            break;
        rec = (S_actor_record *)((uchar *)rec + records->record_sizes[rec->type]);
    }
    S_actor_record *next = prev->time < records->last_key_time ?
        (S_actor_record *)((uchar *)prev + records->record_sizes[prev->type]) : prev;
    if (time <= prev->time)
        return prev->rot;
    if (next != prev && time <= next->time)
    {
        float len = (float)(next->time - prev->time);
        float f = (time - prev->time) / len;
        S_quat rot = prev->rot;
        rot = rot.Slerp(next->rot, f, true);
        return rot;
    }
    return next->rot;
}

// True here (PS2's the same); the original's slots point at the folded copy of the out-of-line kind (0x4aa490, C_actor.h)
bool C_actor::SaveData(C_chunk *chunk)
{
    return true;
}

bool C_actor::LoadData(C_chunk *chunk)
{
    return true;
}
