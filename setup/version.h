#ifndef SETUP_VERSION_H
#define SETUP_VERSION_H

// The setups differ by the original whose code a build has, named by its link date: SETUP_DATE, which the build sets
// to one of these (setup/CMakeLists.txt), 1.2's without it. Both the code and the resource script read it.
// 1.0: its texts in the code and in the dialog
#define SETUP_DATE_1_0 20020728
// Steam's Czech one: the texts from the string table, set at WM_INITDIALOG
#define SETUP_DATE_CZECH 20020823
// Steam's Polish one: the window's caption from the string table too
#define SETUP_DATE_POLISH 20021030
// 1.2, and with it Steam's Russian one and its 2010 rebuild: the W-buffer test and the new multisampling
#define SETUP_DATE_1_2 20021107

#ifndef SETUP_DATE
#define SETUP_DATE SETUP_DATE_1_2
#endif

#endif
