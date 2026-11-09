#pragma once

#include "Main.h"

#include <string.h>
#include <vector>

struct S_item_def;

struct S_item_def;

// An item an actor carries (PS2 S_GameItem), 0x10 bytes (std::vector<S_GameItem>'s size divides by 16): the script's
// HUMAN_ADDWEAPON fills the first three, its CREATEWEAPONFROMFRAME all four (the names are ours but for id)
struct S_GameItem
{
    ushort id;                          // 0x0, the item's index in gameItemTable (g_material.h)
    // 0x2, the clip dropped or the revolver's shells out (C_human::StartWeaponEffect), once a reload; C_human::AI
    // clears it at the right hand's fire's end (the name is ours)
    ushort clip_out;
    // The weapon's ammunition, in it and in reserve: PS2's G_Inventory::Vystrel ("shot") takes one off the first,
    // NabijJeden ("load one") moves one from the second to the first, up to the weapon's gameItemTable count
    int ammo_loaded;                    // 0x4
    int ammo_spare;                     // 0x8
    void *using_object;                 // 0xc, the using object of a weapon put down, NULL in a hand

    // Cleared, as PS2's memset before an item is filled: G_Inventory's constructor constructs its arrays through it
    // FUNCTION: GAME 0x47aed0
    S_GameItem()
    {
        memset(this, 0, sizeof(*this));
    }
};

// G_Inventory's places, Select's kinds (the names are ours): the hand, a pocket, the big weapon's, an item's, the car's
#define INVENTORY_HAND 0
#define INVENTORY_POCKET 1
#define INVENTORY_BIG_WEAPON 2
#define INVENTORY_ITEM 3
#define INVENTORY_CAR 4
// How many pockets and items (the names are ours)
#define INVENTORY_POCKETS 5
#define INVENTORY_ITEMS 4

// SaveGameSave's first dword: SetInCar's state with the big weapons' (the name is ours)
#define SAVE_INVENTORY_IN_CAR 0x80

// The items in a saved game, after the flags' dword (the names are ours)
struct S_inventory_save
{
    S_GameItem right_hand;              // 0x0
    S_GameItem car_item;                // 0x10
    S_GameItem pockets[INVENTORY_POCKETS];  // 0x20
    S_GameItem big_weapon;              // 0x70
    S_GameItem items[INVENTORY_ITEMS];  // 0x80
};

// An actor's inventory (PS2 G_Inventory, g_inventory.cpp), 0xe4 bytes, the size C_game::InventoryStackPush (0x5b1e10)
// allocates a copy of a human's (C_human's 0x480) with. PS2's starts with the items, a gif::vector. Its functions lie in
// g_loadsave.cpp's range on the PC (ui/g_loadsave.cpp).
class G_Inventory
{
public:
    // C_human's constructor inlines it, the items constructed and Clear called out of line
    G_Inventory()
    {
        Clear();
    }
    // A copy of the inventory's items, not of its cycle (C_game::InventoryStackPush's, as PS2's): the copy's cycle is
    // Clear's
    G_Inventory(const G_Inventory &inv)
    {
        Clear();
        can_have_big_weapon = inv.can_have_big_weapon;
        in_car = inv.in_car;
        num_items = inv.num_items;
        right_hand = inv.right_hand;
        int i;
        for (i = 0; i < INVENTORY_POCKETS; i++)
            pockets[i] = inv.pockets[i];
        big_weapon = inv.big_weapon;
        for (i = 0; i < INVENTORY_ITEMS; i++)
            items[i] = inv.items[i];
        car_item = inv.car_item;
        messages = inv.messages;
    }

    // PS2 names, which the script calls; the parameters' names are ours. The items the hands drop go into the vector.
    // Selects the item for the hand
    bool SelectByID(uint id, std::vector<S_GameItem> *dropped);
    // Puts the hand's weapon away, the items that don't fit into dropped (PS2 name; the bools' names are ours,
    // C_human::Do_Holster passes false and whether the human is the player)
    void Holster(std::vector<S_GameItem> *dropped, bool keep, bool player);
    // Clears the item at idx of the place where, INVENTORY_* (PS2 name; the parameters' names are ours), and closes
    // up the gaps
    void RemoveByIdx(uint idx, uint where);
    // Whether the hand's item, or without in_hand any item, is of gameItemTable's flags
    bool HaveSomeWeapon(bool in_hand, uint flags);
    // Whether the inventory has an item of the id, in the hand only with in_hand
    bool IsIn(uint id, bool in_hand);
    // Whether an item of the id has a place, or merges with one the inventory has; with_hand the empty hand counts
    bool HaveSpaceFor(uint id, bool with_hand);
    // Puts the item away: a weapon already there takes its ammunition, unless no_merge (the name is ours)
    bool Insert(S_GameItem *item, bool no_merge);
    bool Remove(S_GameItem *item, std::vector<S_GameItem> *dropped);
    void SetCanHaveBigWeapon(bool can, std::vector<S_GameItem> *dropped);
    // Into a car or out of it: the items the inventory can't keep in a car go into dropped (PS2 name; the
    // parameters' names are ours)
    void SetInCar(bool in, std::vector<S_GameItem> *dropped);
    // The right hand's item's gameItemTable entry, the table's first for an id past its end (PS2 name, 0x607b80)
    S_item_def *GetWeaponInRightHand();
    // The left hand's (PS2 name)
    S_item_def *GetWeaponInLeftHand();
    // The inventory in a saved game (the two return true)
    int SaveGameGetSize();
    bool SaveGameSave(void **buf);
    bool SaveGameLoad(void **buf);
    // The right hand's weapon's ammunition in it, and in reserve into spare (PS2 names, C_human::AI's; the
    // parameter's name is ours)
    uint GetAmmo(int *spare);
    // The weapon loaded from the reserve (Czech "load"), one round of it ("load one"), and whether one can be
    // ("can load one")
    bool Nabij();
    bool NabijJeden();
    // A round taken off the right hand's weapon (Czech "shot", PS2 name): false where it has none
    bool Vystrel();
    bool MuzeNabitJeden();
    // PS2 names; the parameters' names are ours
    // Takes the item of the place of that kind (INVENTORY_*) and index into the hand, the hand's item into the place
    // or elsewhere, into dropped what has no place: false for a kind out of range
    bool Select(uint index, uint kind, std::vector<S_GameItem> *dropped);
    // Fills the list with the items of gameItemTable's flags, the car's but; their count
    uint HaveItems(std::vector<S_GameItem> *list, uint flags);
    // Brings cycle up to date, and the next or the previous weapon in it into the hand
    void CyclePrepare();
    bool CycleUp(std::vector<S_GameItem> *dropped);
    bool CycleDown(std::vector<S_GameItem> *dropped);
    // The ammunition of from added to the item's reserve, up to its most (Czech "top up the rounds"): true if from has
    // none left
    bool DobijNaboje(S_GameItem *item, S_GameItem *from);
    // The item into a free place but the hand: big_weapon, a pocket or items, as its flags allow; false if there is
    // none (InsertNew's and Insert's helper; the name is ours)
    bool InsertFree(S_GameItem *item);
    bool InsertNew(S_GameItem *item, std::vector<S_GameItem> *dropped);
    // The items a body leaves to take into list (PS2 name)
    void EnemyToList(std::vector<S_GameItem *> *list);
    void RemoveByPtr(S_GameItem *item);
    uint GetAmmoByID(uint id, int *spare);
    void _Sesyp();

    // The inventory's places (the names are ours): the right hand, the pockets for the weapons that aren't big, the
    // place for a big weapon, the other items' and, in a car, the car's for what has no other place (Select's kinds)
    // 0x0, big_weapon takes a big weapon (SetCanHaveBigWeapon, 1 from Clear), an ITEMF_SHORT one even without it
    int can_have_big_weapon;
    int in_car;                         // 0x4, SetInCar's: car_item holds an item
    uint num_items;                     // 0x8, how many of items hold one, at their front
    // 0xc, the ids the weapons cycle through (CycleUp, CycleDown): the hand's weapon and the stored ones', in the
    // order they came, which CyclePrepare brings up to date
    std::vector<uint> cycle;
    uint cycle_id;                      // 0x1c, the id the cycle is at, 0xffffff from Clear
    S_GameItem right_hand;              // 0x20, the item in the right hand
    S_GameItem pockets[INVENTORY_POCKETS];  // 0x30, the weapons (ITEMF_WEAPON or ITEMF_MELEE) that aren't big
    S_GameItem big_weapon;              // 0x80, a big one (ITEMF_BIG)
    S_GameItem items[INVENTORY_ITEMS];  // 0x90, the items that aren't weapons, num_items of them
    S_GameItem car_item;                // 0xd0, in a car, the hand's item that has no other place
    // 0xe0, the console's messages of Holster and DobijNaboje: no place to holster the weapon, a weapon the
    // inventory has already, or whose ammunition it took
    bool messages;

    // The accessors whose copies are among those at the end of the file at 0x460a50 (the names are ours)
    // Empties the inventory, the script's INVENTORY_CLEAR (PS2's goes on with an empty item and CyclePrepare)
    // FUNCTION: GAME 0x47b020
    void Clear()
    {
        can_have_big_weapon = 1;
        in_car = 0;
        num_items = 0;
        cycle_id = 0xffffff;
        memset(&right_hand, 0, sizeof(right_hand));
        memset(pockets, 0, sizeof(pockets));
        memset(&big_weapon, 0, sizeof(big_weapon));
        memset(items, 0, sizeof(items));
        memset(&car_item, 0, sizeof(car_item));
        cycle.clear();
        messages = false;
    }
    // The right hand's item's id, and the item where asked for: a uint, which the copy widens (xor eax, eax; mov ax)
    // and the script's HUMAN_GETITEMINRHAND converts as unsigned (fild qword)
    // FUNCTION: GAME 0x47b0b0
    uint GetItemInRightHand(S_GameItem **item)
    {
        if (item)
            *item = &right_hand;
        return right_hand.id;
    }
};

// The arrays' constructions, which C_human's constructor calls for the inventory's (the compiler's helper)
// SYNTHETIC: GAME 0x403d80
// `vector constructor iterator'

// Its destructor's copy, which C_game::InventoryStackClear calls: the original kept the copy of the range at 0x53e4b0
// (docs/game-files.md) of its folded group
// SYNTHETIC: GAME 0x53ea20
// ??1G_Inventory@@QAE@XZ

// The items' insert, which C_human's Do_WeaponDrop and the others' inlined push_backs call: the original kept the copy at
// 0x438f00 of its folded group
// TEMPLATE: GAME 0x438f00
// ?insert@?$vector@US_i_show_fade@C_game@@V?$allocator@US_i_show_fade@C_game@@@std@@@std@@QAEXPAUS_i_show_fade@C_game@@IABU34@@Z
