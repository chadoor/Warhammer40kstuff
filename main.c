#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>

#define MAX_UNIT_SIZE 10
#define MAX_MODEL_SIZE 30
#define NAME_LENGTH 30
#define DIE_MINIMUM 1
#define D3 3
#define D6 6

#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_RESET   "\x1b[0m"

typedef struct {
   char weapon_name[NAME_LENGTH];
   uint8_t ballistic_skill;
   uint8_t strenght;
   int8_t armor_piercing;
   uint8_t damage;
   uint8_t damage_die;
} Weapon;

typedef struct {
    char model_name[NAME_LENGTH];
    uint8_t movement;
    uint8_t tougness;
    uint8_t armor_save;
    int8_t wound;
    int8_t max_wound;
    int8_t leadership;
    uint8_t invulnerable_save;
    Weapon weapon;
} Model;

typedef struct{
    char unit_name[NAME_LENGTH];
    Model models[MAX_MODEL_SIZE];
    int8_t model_count;
} Unit;

typedef struct {
    char army_name[NAME_LENGTH];
    Model models[MAX_MODEL_SIZE];
    Unit units[MAX_UNIT_SIZE];
    int8_t model_count;
    int8_t unit_count;
} Army;

uint8_t roll_d6(){
    return 1 +rand() % 6;
}

uint8_t roll_d3(){
    return 1 +rand() % 3;
}


//   Utilities 
// ------------------------

void print_unit(Unit *unit){
    printf("Unit Name: %s \n", unit->unit_name);
    printf("Models in this unit:\n");
    for(uint8_t i = 0; i < unit->model_count; i++){
        printf("  Model %d:\n", i+1);
        printf("    Name: %s \n", unit->models[i].model_name);
        printf("    Health: %d \n", unit->models[i].wound);
        printf("    Weapon: %s \n", unit->models[i].weapon.weapon_name);
    }
    printf("\n");
}

void print_army(Army *army){
    printf("Army Name: %s \n", army->army_name);
    printf("Units in this army:\n");
    for(uint8_t i = 0; i < army->unit_count; i++){
        printf("  Unit %d:\n", i+1);
        printf("    Name: %s \n", army->units[i].unit_name);
        printf("    Models: %d \n", army->units[i].model_count);
    }
    printf("\n");
}

void select_model1(Unit *unit){
    printf("Select Model: \n");
    for(uint8_t i = 0; i < unit->model_count; i++){
        printf("%d - %s \n",i,unit->models[i].model_name);
    }
}

void select_model(Unit *unit){
    int model_index;
    printf("Enter the index of the model you want to select (0 to %d): ", unit->model_count - 1);
    scanf("%d", &model_index);

    if(model_index >= 0 && model_index < unit->model_count){
        Model selected_model = unit->models[model_index];
        printf("You selected model %d: %s\n", model_index, selected_model.model_name);
    } else {
        printf("Invalid model index. Please enter a number between 0 and %d.\n", unit->model_count - 1);
    }
}

int8_t dead_models = 0;


/*
    Game Loop
    1-Teams selected
    2-Combat starts
*/


enum Turn{
    RED,
    BLUE
};

uint8_t get_wounding_threshold(uint8_t strenght, uint8_t tougness){
    if(strenght >= tougness * 2){
        return 2;
    }
    if(strenght > tougness){
        return 3;
    }
    if(strenght == tougness){
        return 4;
    }
    if(strenght < tougness){
        return 5;
    }
    if(strenght * 2 <= tougness){
        return 6;
    }
    return 0;
}

/*
    ------------ Combat ------------ 
    1- Attemt to Hit
        roll 1 D6 then check the wepons balistick skill or weapon skill. 
        if the number is equal or greater you hit the target.
        Continue to Wound  section.

    2- Attemt to Wound
        get weapons strengh versus opponents toughness with *get_wounding_threshold() function.
        roll 1 D6 then check the result from *get_wounding_threshold(). 
        if  is equal or greater you wound the target.
        Continue to Armor Save seciton.
            
    3- Attemt to Armor Save
        roll 1 D6 then check the armor save value on the defender.
        if the number is equal or greater defender has succesfully saved againts the attack.
        if not continue to Damage section.

    4- Deal Damage
       Subtract the damage value of the weapon from the defenders wounds.
       if the wounds are equal or less then 0 the defender is dead.
*/

/*
    Weapon Damage
        can be integers 1,2,3,...
        can be dice rolls d3,d6
        can be both d3 + 3,...
*/
uint8_t get_damage_die(uint8_t damage_die){
    if(damage_die == D3){
        return roll_d3();
    }else if (damage_die == D6){
        return roll_d6();
    }
    return 0;
}

void handle_damage_die(Model *attacker,uint8_t *damage_die_value){
    uint8_t attacker_damage_die = attacker->weapon.damage_die;
    if(attacker_damage_die){
        *damage_die_value = get_damage_die(attacker_damage_die);
    }
}

void handle_damage(Model *attacker,Model *defender){
    uint8_t damage_die_value = 0;
    handle_damage_die(attacker,&damage_die_value);
    uint8_t damage_value = attacker->weapon.damage + damage_die_value;
    char *defender_name = defender->model_name;
    defender->wound -= damage_value;
    if(defender->wound <= 0){
        defender->wound = defender->max_wound;
        dead_models++;
        printf(ANSI_COLOR_BLUE "%s takes %d wound and dies \n" ANSI_COLOR_RESET,defender_name,damage_value);
    }
    else {
        printf(ANSI_COLOR_BLUE "%s takes %d wound \n" ANSI_COLOR_RESET,defender_name,damage_value);
    }
}

void handle_invulnerable_save(Model *defender, uint8_t *armor_save_value){
    uint8_t invulnerable_save_value = defender->invulnerable_save;
    if(invulnerable_save_value && invulnerable_save_value < *armor_save_value){
       *armor_save_value = invulnerable_save_value;
    }
}

void handle_save(Model *attacker,Model *defender){
    int8_t save_roll = roll_d6();
    uint8_t armor_save_value = defender->armor_save;
    uint8_t attacker_armor_piercing_value = attacker->weapon.armor_piercing;
    //todo check armor_piercing_value if its beeng aplied correctly.
    armor_save_value -= attacker_armor_piercing_value;
    char *defender_name = defender->model_name;
    handle_invulnerable_save(defender,&armor_save_value);
    if(save_roll >= armor_save_value){
        printf(ANSI_COLOR_BLUE "%s saves with %d (%d/%d)!\n" ANSI_COLOR_RESET,defender_name,save_roll,save_roll,armor_save_value);
    } else {
        printf(ANSI_COLOR_BLUE "%s failes to save with %d (%d/%d)!\n" ANSI_COLOR_RESET,defender_name,save_roll,save_roll,armor_save_value);
        handle_damage(attacker,defender);
    }
}

void handle_wound(Model *attacker,Model *defender){
    uint8_t wound_roll = roll_d6();
    uint8_t attacker_strength = attacker->weapon.strenght;
    uint8_t defender_toughness = defender->tougness;
    uint8_t wound_val = get_wounding_threshold(attacker_strength,defender_toughness);
    char *attacker_name = attacker->model_name;
    char *defender_name = defender->model_name;
    char *weapon_name = attacker->weapon.weapon_name;
    if(wound_roll >= wound_val){
        printf(ANSI_COLOR_RED "%s wounds Enemy %s  with %s (%d/%d) \n" ANSI_COLOR_RESET,attacker_name,defender_name,weapon_name,wound_roll,wound_val);
        handle_save(attacker,defender);
    }else {
        printf(ANSI_COLOR_RED "%s failes to wound  Enemy %s  with %s (%d/%d) \n" ANSI_COLOR_RESET,attacker_name,defender_name,weapon_name,wound_roll,wound_val);
    }
}

void handle_attack(Model *attacker,Model *defender){
    uint8_t attack_roll = roll_d6();
    uint8_t attacker_balistic_skill = attacker->weapon.ballistic_skill;
    char *attacker_name = attacker->model_name;
    char *defender_name = defender->model_name;
    char *weapon_name =attacker->weapon.weapon_name;
    if( attack_roll >= attacker_balistic_skill){
        printf(ANSI_COLOR_RED "%s hits  Enemy %s  with %s (%d/%d)\n" ANSI_COLOR_RESET,attacker_name,defender_name,weapon_name,attack_roll,attacker_balistic_skill);
        handle_wound(attacker,defender);
    }else {
        printf(ANSI_COLOR_RED "%s misses  Enemy %s with %s (%d/%d)\n" ANSI_COLOR_RESET,attacker_name,defender_name,weapon_name,attack_roll,attacker_balistic_skill);
    }
}

void model_combat(Model *attacker,Model * defender,enum Turn current_turn){
    printf("------- Attack Sequence -------- \n");
    if(current_turn == RED){
         handle_attack(attacker,defender);
    } else {
         handle_attack(defender,attacker);
    }
    printf("------- Attack Sequence Ends -------- \n \n");
}


void unit_combat(Unit *attacker, Unit *defender, enum Turn current_turn){
    if(defender->model_count <= 0) {
        printf("No models in the defending unit to attack.\n");
        return;
    }

    for(uint8_t i = 0; i < attacker->model_count; i++){
        model_combat(&attacker->models[i], &defender->models[0], current_turn);
    }
}


void army_combat(Army *attacker, Army *defender, enum Turn current_turn){
    if(defender->unit_count <= 0) {
        printf("No units in the defending army to attack.\n");
        return;
    }

    for(uint8_t i = 0; i < attacker->unit_count; i++){
        unit_combat(&attacker->units[i], &defender->units[0], current_turn);
    }

}

// void start_combat(Army *army1, Army *army2,enum Turn current_turn){
//     printf("------- Combat Sequence -------- \n");
//     if(current_turn == RED){
//         unit_combat(army1,army2);
//     } else {
//         unit_combat(army2,army1);
//     }
//     printf("------- Combat Sequence Ends -------- \n \n");
// }



void remove_dead_units(Army *army, uint8_t size ,uint8_t index, enum Turn current_turn){
    if(army->models[index].wound > 0){
        return;
    }

    if (current_turn == RED){
        //BLUE
        printf(ANSI_COLOR_BLUE "%s has died !\n" ANSI_COLOR_RESET,army->models[index].model_name);
    } else if (current_turn == BLUE){
        //RED
        printf(ANSI_COLOR_RED "%s has died !\n" ANSI_COLOR_RESET,army->models[index].model_name);
    }
    

    for(uint8_t i = index; i < size - 1; i++){
        army->models[i] = army->models[i + 1];
    }

    army->model_count--;

}


int main(void){

    srandom(time(0));

    Weapon pistol = {
        .weapon_name = "AutoPistol",
        .ballistic_skill = 4,
        .strenght = 3,
        .armor_piercing = 0,
        .damage = 1,
        .damage_die = 0
    };

    Weapon rifle = {
        .weapon_name = "Exitus_rifle",
        .ballistic_skill = 2,
        .strenght = 7,
        .armor_piercing = -3,
        .damage = 3,
        .damage_die = 3
    };

    Weapon shotgun = {
        .weapon_name = "Shotgun",
        .ballistic_skill = 4,
        .strenght = 4,
        .armor_piercing = 0,
        .damage = 1,
        .damage_die = 0
    };
    

    Model breachers = {
        .model_name="Navis Armsmen",
        .movement=6,
        .tougness=3,
        .armor_save = 4,
        .wound=1,
        .max_wound=1,
        .leadership=1,
        .weapon = pistol,
        .invulnerable_save = 0
    };

    Model vindicare_assassin = {
        .model_name = "Vindicare Assassin",
        .movement = 7,
        .tougness = 4,
        .armor_save = 6,
        .wound = 4,
        .max_wound = 4,
        .leadership = 6,
        .invulnerable_save = 4,
        .weapon = rifle
    };


    Model exacation_vigilants = {
        .model_name = "Exacation Vigilants",
        .movement = 6,
        .tougness = 3,
        .armor_save = 4,
        .wound = 1,
        .max_wound = 1,
        .leadership = 7,
        .invulnerable_save = 0,
        .weapon = shotgun
    };



    Unit unit1 = {
        .unit_name = "Imperial Navy Breachers",
        .models = {breachers,breachers,breachers,breachers},
        .model_count = 4
    };

    Unit unit2 = {
        .unit_name = "EXACTION SQUAD",
        .models = {exacation_vigilants,exacation_vigilants,exacation_vigilants,exacation_vigilants},
        .model_count = 4
    };

    Unit unit3 = {
        .unit_name = "Vindicare Assassin",
        .models = {vindicare_assassin},
        .model_count = 1
    };
    

    // Unit unit3 = {
    //     .unit_name = "Imperial Navy Breachers 3",
    //     .models = {&breachers_5},
    //     .model_count = 1
    // };

    // Unit unit4 = {
    //     .unit_name = "Vindicare Assassin",
    //     .models = {&vindicare_assassin,},
    //     .model_count = 1
    // };

    // Army army1 = {
    //     .models = {breachers_1,breachers_2,breachers_3},
    //     .model_count = 3,
    //     .army_name = "Agents of Imperium"
    // };
    
    // Army army2 = {
    //     .models = {breachers_4,vindicare_assassin},
    //     .model_count = 2,
    //     .army_name = "Agents of Imperium"
    // };

    Army army1 = {
        .units = {unit1,unit3},
        .unit_count = 2,
        .army_name = "Agents of Imperium"
    };
    
    Army army2 = {
        .units = {unit2},
        .unit_count = 1,
        .army_name = "Agents of Imperium"
    };

    enum Turn this_turn = RED;
    
    printf("First Round ! \n");
    while (0){
        if(army1.model_count <= 0 || army2.model_count <= 0){
            break;
        }
        
        for(uint8_t i = 0; i < army1.model_count; i++){
            for(uint8_t j = 0; j < army2.model_count; j++){
                //start combat
                if(this_turn == RED){
                    printf(ANSI_COLOR_RED "------- %s's  turn -------- \n \n"ANSI_COLOR_RESET,army1.army_name);
                }else {
                    printf(ANSI_COLOR_BLUE "------- %s's  turn -------- \n \n"ANSI_COLOR_RESET,army2.army_name);
                }
                //model_combat(&army1.models[i],&army2.models[j],this_turn);

                army_combat(&army1,&army2,this_turn);

                //remove dead models
                remove_dead_units(&army1,army1.model_count,i,this_turn);        
                remove_dead_units(&army2,army2.model_count,j,this_turn);

                //  flip turn order
                this_turn = (this_turn == RED ) ? BLUE : RED;

                if(army1.model_count <= 0 || army2.model_count <= 0){
                    break;
                }
            }
        }
    }

    // print_unit(&unit1);
    // select_model(&unit2);
    // print_army(&army2);    
    // print_unit(&unit2);
    
    army_combat(&army2,&army1,this_turn);
    print_army(&army1);
    print_unit(&unit1);
    printf("End of Combat \n");
}