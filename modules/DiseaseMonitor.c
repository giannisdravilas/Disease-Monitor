#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>

#include "ADTMap.h"
#include "ADTSet.h"
#include "ADTPriorityQueue.h"

#include "DiseaseMonitor.h"

// Hash συνάρτηση για το παρακάτω struct (δεν μπορούμε να χρησιμοποιήσουμε τη hash_pointer() γιατί
// η σύγκριση γίνεται με βάση τα περιεχόμενα του struct, οπότε σε μελλοντική αναζήτηση με τη χρήση
// ενός νέου struct θα οδηγηθούμε σε λανθασμένη hash_position λόγω διαφορετικής θέσης μνήμης)
uint hash_struct_disease_country_date(Pointer value);

// Δομή που αποθηκεύει συνδυασμούς disease, country, date για χρήση στην dm_count_records()
struct map_count{
    String disease;
    String country;
    Date date;
};

// Ο τύπος MapCount είναι δείκτης σε δομή map_count
typedef struct map_count* MapCount;

// Τα βασικά Maps στα οποία αποθηκεύονται οι πληροφορίες και που θα χρησιμοποιηθούν για την dm_get_records()
static Map map_ids_records;
static Map map_diseases_dates;
static Map map_countries_dates;

// Λογική μεταβλητή με την οποία θα ξεχωρίζουμε την αναζήτηση των ημερομηνιών στη dm_get_records()
static bool find = false;

int compare_ints(Pointer a, Pointer b){
    return *(int*)a - *(int*)b;
}

int compare_diseases(Pointer a, Pointer b){
    return strcmp(a, b);
}

int compare_countries(Pointer a, Pointer b){
    return strcmp(a, b);
}

// Συγκρίνουμε με βάση την ημερομηνία και αν είναι ίδια με βάση το id
// Στην περίπτωση που η find == true, δηλαδή η αναζήτηση γίνεται για την dm_get_records(),
// τότε συγκρίνουμε μόνο με την ημερομηνία, αφού απλώς ψάχνουμε κάποια εγγραφή με ίδια ημερομηνία
int compare_record_dates(Pointer a, Pointer b){
    Record record_a = a;
    Record record_b = b;
    if(find)
        return strcmp(record_a->date, record_b->date);
    if(strcmp(record_a->date, record_b->date)){
        return strcmp(record_a->date, record_b->date);
    }else{
        return (record_a->id)-(record_b->id);
    }
}

int compare_dates(Pointer a, Pointer b){
    return strcmp(a, b);
}

// Το Map που θα χρησιμοποιηθεί για την dm_count_records()
static Map map_disease_country_date;

// Συνάρτηση compare για τα δεδομένα του Map της dm_count_records()
int compare_structs(Pointer a, Pointer b){
    MapCount mapcount_a = a;
    MapCount mapcount_b = b;
    if(!strcmp(mapcount_a->disease, mapcount_b->disease)){
        if(!strcmp(mapcount_a->country, mapcount_b->country)){
            return strcmp(mapcount_a->date, mapcount_b->date);
        }else{
            return strcmp(mapcount_a->country, mapcount_b->country);
        }
    }else{
        return strcmp(mapcount_a->disease, mapcount_b->disease);
    }
}

// Το Map που θα χρησιμοποιηθεί για την dm_top_diseases() αν country != NULL
static Map map_country_diseases;

// Συνάρτηση compare για την Priority Queue - Συγκρίνει με βάση τα values κάθε κόμβου στο map_country_diseases()
int compare_diseases_counter(Pointer a, Pointer b){
    return *(int*)map_find(map_country_diseases, a) - *(int*)map_find(map_country_diseases, b);
}

// Το Map που θα χρησιμοποιηθεί για την dm_top_diseases() αν country == NULL
static Map map_NULL_diseases;

// Συνάρτηση compare για την Priority Queue - Συγκρίνει με βάση τα values κάθε κόμβου στο map_NULL_diseases()
int compare_NULL_diseases(Pointer a, Pointer b){
    return *(int*)map_find(map_NULL_diseases, a) - *(int*)map_find(map_NULL_diseases, b);
}

int* create_int(int value) {
	int* p = malloc(sizeof(int));
	*p = value;
	return p;
}

void dm_init(){
    map_ids_records = map_create(compare_ints, NULL, NULL);
    map_set_hash_function(map_ids_records, hash_int);
    map_diseases_dates = map_create(compare_diseases, NULL, NULL);
    map_set_hash_function(map_diseases_dates, hash_string);
    map_countries_dates = map_create(compare_countries, NULL, NULL);
    map_set_hash_function(map_countries_dates, hash_string);

    map_disease_country_date = map_create(compare_structs, NULL, NULL);
    map_set_hash_function(map_disease_country_date, hash_struct_disease_country_date);

    map_NULL_diseases = map_create(compare_diseases, NULL, NULL);
    map_set_hash_function(map_NULL_diseases, hash_string);

}

void dm_destroy(){
    
    // Απελευθερώνουμε πρώτα τη μνήμη που καταλαμβάνει κάθε στοιχείο (Set) του Map
    for(MapNode node = map_first(map_diseases_dates); node != MAP_EOF; node = map_next(map_diseases_dates, node)){
        set_destroy(map_node_value(map_diseases_dates, node));
    }
    for(MapNode node = map_first(map_countries_dates); node != MAP_EOF; node = map_next(map_countries_dates, node)){
        set_destroy(map_node_value(map_countries_dates, node));
    }

    // Κι έπειτα διαγράφουμε τα Maps
    map_destroy(map_ids_records);
    map_destroy(map_diseases_dates);
    map_destroy(map_countries_dates);

    // Διαγράφουμε τα structs που περιέχει για keys το Map. Χρησιμοποιούμε μια μεταβλητή next γιατί η
    // map_next() δεν θα δουλέψει, αφού το τρέχουν κλειδί θα έχει διαγραφεί όταν αυτή κληθεί
    MapNode next;
    for(MapNode node = map_first(map_disease_country_date); node != MAP_EOF; node = next){
        next = map_next(map_disease_country_date, node);
        MapCount mapcount = map_node_key(map_disease_country_date, node);
        free(mapcount->disease);
        free(mapcount->country);
        free(mapcount->date);
        free(map_node_key(map_disease_country_date, node));
        free(map_node_value(map_disease_country_date, node));
    }

    // Έπειτα διαγράφουμε το ίδιο το Map
    map_destroy(map_disease_country_date);

    // Διαγράφουμε πρώτα τα values του Map
    for(MapNode node = map_first(map_NULL_diseases); node != MAP_EOF; node = map_next(map_NULL_diseases, node)){
        free(map_node_value(map_NULL_diseases, node));
    }

    // Κι έπειτα το ίδιο το Map
    map_destroy(map_NULL_diseases);
}

// Για ενημέρωση του map_disease_country_date
void update_map_count(Map map, Record record){

    //Δημιουργούμε ένα mapcount με τα στοιχεία που θέλουμε να εισαγάγουμε στο Map
    MapCount mapcount = malloc(sizeof(*mapcount));
    mapcount->disease = strdup(record->disease);
    mapcount->country = strdup(record->country);
    mapcount->date = strdup(record->date);

    MapNode found = map_find_node(map, mapcount);

    // Αν υπάρχει ήδη key με αυτά τα στοιχεία, τότε αυξάνουμε το value
    if(found){
        Pointer value = map_node_value(map, found);
        (*(int *)value)++;
        free(mapcount->disease);
        free(mapcount->country);
        free(mapcount->date);
        free(mapcount);

    // Αλλιώς εισάγουμε ένα νέο key με αυτά τα στοιχεία και value 1 αφού είναι η πρώτη φορά
    // που εισάγεται στο map_disease_country_date key με αυτά τα στοιχεία
    }else{
        map_insert(map, mapcount, create_int(1));
    }
}

bool dm_insert_record(Record record){

    MapNode found_node = map_find_node(map_ids_records, &(record->id));

    // Αν υπάρχει ήδη record με το ίδιο id
    if(found_node){

        // Βρίσκουμε το προηγούμενο record με το ίδιο id
        Record value = map_node_value(map_ids_records, found_node);

        // Αφαιρούμε από τα map_disease_country_date και map_NULL_disease ένα στοιχείο με τα
        // στοιχεία του προηγούμενου record. H map_find() δεν υπάρχει περίπτωση να επιστρέψει
        // NULL αφού σίγουρα θα έχει εισαχθεί τέτοιο στοιχείο και στα δύο Maps
        MapCount mapcount = malloc(sizeof(*mapcount));
        mapcount->disease = value->disease;
        mapcount->country = value->country;
        mapcount->date = value->date;
        (*(int*)map_find(map_disease_country_date, mapcount))--;
        (*(int*)map_find(map_NULL_diseases, value->disease))--;
        free(mapcount);

        // Αφαιρούμε και επανεισάγουμε το αντίστοιχο record και από τα υπόλοιπα maps, εκείνα
        // δηλαδή που χρησιμοποιεί η map_get_records()
        map_remove(map_ids_records, &(record->id));
        map_insert(map_ids_records, &(record->id), record);

        MapNode found = map_find_node(map_diseases_dates, record->disease);
        if(found){
            set_remove(map_node_value(map_diseases_dates, found), record);
            set_insert(map_node_value(map_diseases_dates, found), record);
        }

        found = map_find_node(map_countries_dates, record->country);
        if(found){
            set_remove(map_node_value(map_countries_dates, found), record);
            set_insert(map_node_value(map_countries_dates, found), record);
        }

        // Ενημερώνουμε και τα map_disease_country_date, map_NULL_diseases
        update_map_count(map_disease_country_date, record);

        Pointer value1 = map_find(map_NULL_diseases, record->disease);
        if(value){
            (*(int*)value1)++;
        }else{
            map_insert(map_NULL_diseases, record->disease, create_int(1));
        }

        return true;

    // Αν δεν υπάρχει record με το ίδιο id    
    }else{

        // Εισάγουμε το record στο map_ids_records
        map_insert(map_ids_records, &(record->id), record);

        // Κάθε στοιχείο του map_diseases_dates είναι ένα Set. Αν υπάρχει Set εισάγουμε
        // το record σε αυτό, αλλιώς δημιουργούμε ένα Set και έπειτα εισάγουμε το record.
        MapNode found = map_find_node(map_diseases_dates, record->disease);
        if(found){
            set_insert(map_node_value(map_diseases_dates, found), record);
        }else{
            Set set = set_create(compare_record_dates, NULL);
            map_insert(map_diseases_dates, record->disease, set);
            set_insert(set, record);
        }

        // Κάθε στοιχείο του map_countries_dates είναι ένα Set. Αν υπάρχει Set εισάγουμε
        // το record σε αυτό, αλλιώς δημιουργούμε ένα Set και έπειτα εισάγουμε το record.
        found = map_find_node(map_countries_dates, record->country);
        if(found){
            set_insert(map_node_value(map_countries_dates, found), record);
        }else{
            Set set = set_create(compare_record_dates, NULL);
            map_insert(map_countries_dates, record->country, set);
            set_insert(set, record);
        }

        // Ενημερώνουμε και τα map_disease_country_date, map_NULL_diseases
        update_map_count(map_disease_country_date, record);
        Pointer value = map_find(map_NULL_diseases, record->disease);
        if(value){
            (*(int*)value)++;
        }else{
            map_insert(map_NULL_diseases, record->disease, create_int(1));
        }

        return false;
    }
}

bool dm_remove_record(int id){

    // Βρίσκουμε το record με το συγκεκριμένο id
    Record record = map_find(map_ids_records, &id);

    // Αν όντως υπάρχει
    if(record){

        // Βρίσκουμε την εγγραφή με τα αντίστοιχα στοιχεία στα map_disease_country_date,
        // map_NULL_diseases και μειώνουμε τα values τους κατά 1
        MapCount mapcount = malloc(sizeof(*mapcount));
        mapcount->disease = record->disease;
        mapcount->country = record->country;
        mapcount->date = record->date;
        (*(int*)map_find(map_disease_country_date, mapcount))--;
        (*(int*)map_find(map_NULL_diseases, record->disease))--;

        // Αφαιρούμε το record και από τα Maps που χρησιμποιεί η map_get_records()
        set_remove(map_node_value(map_diseases_dates, map_find_node(map_diseases_dates, record->disease)), record);
        set_remove(map_node_value(map_countries_dates, map_find_node(map_countries_dates, record->country)), record);
        map_remove(map_ids_records, &id);

        free(mapcount);

        return true;
    }
    return false;
}

///////////////////////////////////////////////////////////////////////
///////////// Συναρτήσεις για χρήση στην dm_get_records() /////////////
///////////////////////////////////////////////////////////////////////

// Βρίσκει τον κόμβο του Set που το record του έχει ημερομηνία date_from
SetNode get_date_from_node(Set set, Date date_from){

    // Αν date_from == NULL επιστρέφουμε τον πρώτο κόμβο του Set, αφού η dm_get_records()
    // θα πρέπει να επιστρέψει όλα τα records μέχρι date_to
    if(!date_from){
        return set_first(set);
    }else{
        Record record = malloc(sizeof(*record));
        record->date = strdup(date_from);

        // Αλλάζουμε προσωρινά το find σε true ώστε η CompareFunc να συγκρίνει αποκλειστικά με
        // βάση τις ημερομηνίες και όχι τα ids
        find = true;
        SetNode node = set_find_node(set, record);
        find = false;

        free(record->date);
        free(record);
        return node;
    }
}

// Βρίσκει τον κόμβο του Set που το record του έχει ημερομηνία date_to
SetNode get_date_to_node(Set set, Date date_to){

    // Αν date_to == NULL επιστρέφουμε SET_EOF, αφού η dm_get_records()
    // θα πρέπει να επιστρέψει όλα τα records από date_from και μετά
    if(!date_to){
        return SET_EOF;
    }else{
        Record record = malloc(sizeof(*record));
        record->id = -1;
        record->country = NULL;
        record->disease = NULL;
        record->name = NULL;
        record->date = strdup(date_to);

        // Αλλάζουμε προσωρινά το find σε true ώστε η CompareFunc να συγκρίνει αποκλειστικά με
        // βάση τις ημερομηνίες και όχι τα ids
        find = true;
        SetNode node = set_find_node(set, record);
        find = false;

        // Θέλουμε να επιτρέψουμε τον επόμενο του node, αφού το Set θα πρέπει να διατρεχθεί
        // μέχρι και τον node, οπότε στη συνθήκη της for() θα πρέπει να χρησιμοποιηθεί ο
        // επόμενος κόμβος.
        if(node){
            free(record->date);
            free(record);
            return set_next(set, node);
        }else{
            set_insert(set, record);
            SetNode next = set_next(set, set_find_node(set, record));
            set_remove(set, record);
            free(record->date);
            free(record);
            return next;
        }
    }
}

///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////

List dm_get_records(String disease, String country, Date date_from, Date date_to){
    
    List list = list_create(NULL);
    
    // Διακρίνουμε τις διάφορες περιπτώσεις ανάλογα με το αν είναι NULL κάποιο από τα
    // disease, country, και τα δύο ή κανένα από τα δύο. Κάθε φορά βρίσκουμε το set στο
    // οποίο ανήκει η εγγραφή, καθώς και τους κόμβους του Set που αντιστοιχούν στην
    // date_from και την επόμενη της date_to. Αν η εγγραφή πληροί όλες τις απαραίτητες
    // προϋποθέσεις τότε εισάγεται στη λίστα.
    if(disease){
        Set set = map_find(map_diseases_dates, disease);
        SetNode date_from_node = get_date_from_node(set, date_from);
        SetNode date_to_node = get_date_to_node(set, date_to);
        if(country){
            for(SetNode node = date_from_node; node != date_to_node; node = set_next(set, node)){
                Record record = set_node_value(set, node);
                if(strcmp(record->country, country) == 0){
                    list_insert_next(list, list_last(list), record);
                }
            }
        }else{
            for(SetNode node = date_from_node; node != date_to_node; node = set_next(set, node)){
                Record record = set_node_value(set, node);
                list_insert_next(list, list_last(list), record);
            }
        }
    }else{
        if(country){
            Set set = map_find(map_countries_dates, country);
            SetNode date_from_node = get_date_from_node(set, date_from);
            SetNode date_to_node = get_date_to_node(set, date_to);
            for(SetNode node = date_from_node; node != date_to_node; node = set_next(set, node)){
                Record record = set_node_value(set, node);
                list_insert_next(list, list_last(list), record);
            }
        }else{
            for(MapNode node = map_first(map_diseases_dates); node != MAP_EOF; node = map_next(map_diseases_dates, node)){
                Set set = map_node_value(map_diseases_dates, node);
                if(set_size(set)){
                    SetNode date_from_node = get_date_from_node(set, date_from);
                    SetNode date_to_node = get_date_to_node(set, date_to);
                    for(SetNode node_set = date_from_node; node_set != date_to_node; node_set = set_next(set, node_set)){
                        Record record = set_node_value(set, node_set);
                        list_insert_next(list, list_last(list), record);
                    }
                }
            }
        }
    }

    return list;
}

int dm_count_records(String disease, String country, Date date_from, Date date_to){

    // Σε αυτή τη μεταβλητή αποθηκεύεται το πλήθος των records, που θα αυξάνεται καθώς θα διατρέχουμε το Map
    int counter = 0;
    
    // Διακρίνουμε τις διάφορες περιπτώσεις ανάλογα με το αν είναι NULL κάποιο από τα
    // disease, country, date_from, date_to, όλα ή κανένα. Όταν βρούμε τη σωστή περίπτωση διατρέχουμε το
    // map_disease_country_date που περιέχει περιορισμένο αριθμό εγγραφών (άρα η πολυπλοκότητα δεν εξαρτάται
    // από τον συνολικό αριθμό εγγραφών και αφού τα πλήθη υπάρχουν ήδη ως value και δε χρειάζεται να μετρηθούν
    // δεν εξαρτάται ούτε από τον αριθμό εγγραφών που πληρούν τα κριτήρια) και προσθέτουμε στον counter τα values
    // (πλήθη) των εγγραφών που πληρούν τα κριτήτια. Sorry για τον παρακάτω κώδικα, αλλά δεν μπόρεσα να σκεφτώ κάτι 
    // καλύτερο :P
    if(disease){
        if(country){
            if(date_from && date_to){
                for(MapNode node = map_first(map_disease_country_date); node != MAP_EOF; node = map_next(map_disease_country_date, node)){
                    MapCount mapcount = map_node_key(map_disease_country_date, node);
                    if(!compare_diseases(mapcount->disease, disease) && !compare_countries(mapcount->country, country)){
                        if(compare_dates(mapcount->date, date_from) >= 0 && compare_dates(mapcount->date, date_to) <= 0){
                            counter += *(int*)map_node_value(map_disease_country_date, node);
                        }
                    }
                }
            }else if(date_from){
                for(MapNode node = map_first(map_disease_country_date); node != MAP_EOF; node = map_next(map_disease_country_date, node)){
                    MapCount mapcount = map_node_key(map_disease_country_date, node);
                    if(!compare_diseases(mapcount->disease, disease) && !compare_countries(mapcount->country, country)){
                        if(compare_dates(mapcount->date, date_from) >= 0){
                            counter += *(int*)map_node_value(map_disease_country_date, node);
                        }
                    }
                }
            }else if(date_to){
                for(MapNode node = map_first(map_disease_country_date); node != MAP_EOF; node = map_next(map_disease_country_date, node)){
                    MapCount mapcount = map_node_key(map_disease_country_date, node);
                    if(!compare_diseases(mapcount->disease, disease) && !compare_countries(mapcount->country, country)){
                        if(compare_dates(mapcount->date, date_to) <= 0){
                            counter += *(int*)map_node_value(map_disease_country_date, node);
                        }
                    }
                }
            }else{
                for(MapNode node = map_first(map_disease_country_date); node != MAP_EOF; node = map_next(map_disease_country_date, node)){
                    MapCount mapcount = map_node_key(map_disease_country_date, node);
                    if(!compare_diseases(mapcount->disease, disease) && !compare_countries(mapcount->country, country)){
                        counter += *(int*)map_node_value(map_disease_country_date, node);
                    }
                }
            }
        }else{
            if(date_from && date_to){
                for(MapNode node = map_first(map_disease_country_date); node != MAP_EOF; node = map_next(map_disease_country_date, node)){
                    MapCount mapcount = map_node_key(map_disease_country_date, node);
                    if(!compare_diseases(mapcount->disease, disease)){
                        if(compare_dates(mapcount->date, date_from) >= 0 && compare_dates(mapcount->date, date_to) <= 0){
                            counter += *(int*)map_node_value(map_disease_country_date, node);
                        }
                    }
                }
            }else if(date_from){
                for(MapNode node = map_first(map_disease_country_date); node != MAP_EOF; node = map_next(map_disease_country_date, node)){
                    MapCount mapcount = map_node_key(map_disease_country_date, node);
                    if(!compare_diseases(mapcount->disease, disease)){
                        if(compare_dates(mapcount->date, date_from) >= 0){
                            counter += *(int*)map_node_value(map_disease_country_date, node);
                        }
                    }
                }
            }else if(date_to){
                for(MapNode node = map_first(map_disease_country_date); node != MAP_EOF; node = map_next(map_disease_country_date, node)){
                    MapCount mapcount = map_node_key(map_disease_country_date, node);
                    if(!compare_diseases(mapcount->disease, disease)){
                        if(compare_dates(mapcount->date, date_to) <= 0){
                            counter += *(int*)map_node_value(map_disease_country_date, node);
                        }
                    }
                }
            }else{
                for(MapNode node = map_first(map_disease_country_date); node != MAP_EOF; node = map_next(map_disease_country_date, node)){
                    MapCount mapcount = map_node_key(map_disease_country_date, node);
                    if(!compare_diseases(mapcount->disease, disease)){
                        counter += *(int*)map_node_value(map_disease_country_date, node);
                    }
                }
            }
        }
    }else{
        if(country){
            if(date_from && date_to){
                for(MapNode node = map_first(map_disease_country_date); node != MAP_EOF; node = map_next(map_disease_country_date, node)){
                    MapCount mapcount = map_node_key(map_disease_country_date, node);
                    if(!compare_countries(mapcount->country, country)){
                        if(compare_dates(mapcount->date, date_from) >= 0 && compare_dates(mapcount->date, date_to) <= 0){
                            counter += *(int*)map_node_value(map_disease_country_date, node);
                        }
                    }
                }
            }else if(date_from){
                for(MapNode node = map_first(map_disease_country_date); node != MAP_EOF; node = map_next(map_disease_country_date, node)){
                    MapCount mapcount = map_node_key(map_disease_country_date, node);
                    if(!compare_countries(mapcount->country, country)){
                        if(compare_dates(mapcount->date, date_from) >= 0){
                            counter += *(int*)map_node_value(map_disease_country_date, node);
                        }
                    }
                }
            }else if(date_to){
                for(MapNode node = map_first(map_disease_country_date); node != MAP_EOF; node = map_next(map_disease_country_date, node)){
                    MapCount mapcount = map_node_key(map_disease_country_date, node);
                    if(!compare_countries(mapcount->country, country)){
                        if(compare_dates(mapcount->date, date_to) <= 0){
                            counter += *(int*)map_node_value(map_disease_country_date, node);
                        }
                    }
                }
            }else{
                for(MapNode node = map_first(map_disease_country_date); node != MAP_EOF; node = map_next(map_disease_country_date, node)){
                    MapCount mapcount = map_node_key(map_disease_country_date, node);
                    if(!compare_countries(mapcount->country, country)){
                        counter += *(int*)map_node_value(map_disease_country_date, node);
                    }
                }
            }
        }else{
            if(date_from && date_to){
                for(MapNode node = map_first(map_disease_country_date); node != MAP_EOF; node = map_next(map_disease_country_date, node)){
                    MapCount mapcount = map_node_key(map_disease_country_date, node);
                    if(compare_dates(mapcount->date, date_from) >= 0 && compare_dates(mapcount->date, date_to) <= 0){
                        counter += *(int*)map_node_value(map_disease_country_date, node);
                    }
                }
            }else if(date_from){
                for(MapNode node = map_first(map_disease_country_date); node != MAP_EOF; node = map_next(map_disease_country_date, node)){
                    MapCount mapcount = map_node_key(map_disease_country_date, node);
                    if(compare_dates(mapcount->date, date_from) >= 0){
                        counter += *(int*)map_node_value(map_disease_country_date, node);
                    }
                }
            }else if(date_to){
                for(MapNode node = map_first(map_disease_country_date); node != MAP_EOF; node = map_next(map_disease_country_date, node)){
                    MapCount mapcount = map_node_key(map_disease_country_date, node);
                    if(compare_dates(mapcount->date, date_to) <= 0){
                        counter += *(int*)map_node_value(map_disease_country_date, node);
                    }
                }
            }else{
                for(MapNode node = map_first(map_disease_country_date); node != MAP_EOF; node = map_next(map_disease_country_date, node)){
                    counter += *(int*)map_node_value(map_disease_country_date, node);
                }
            }
        }
    }

    return counter;
}

List dm_top_diseases(int k, String country){

    MapNode node = map_find_node(map_countries_dates, country);

    // Αν βρούμε τη χώρα στο map και αυτή δεν είναι NULL, δημιουργούμε ένα καινούριο map_country_diseases
    // στο οποίο εισάγουμε όλες τις ασθένειες που περιλαμβάνονται στο Set της συγκεκριμένης χώρας.
    // Οι for() διατρέχουν μόνο το set της συγκεκριμένης χώρας, δεν εξαρτώνται δηλαδή από το συνολικό πλήθος
    // εγγραφών, αφού σε πρακτικές χρήσεις θεωρούμε ότι θα υπάρχουν παραπάνω από μία χώρες.
    if(node){
        if(country){
            map_country_diseases = map_create(compare_diseases, NULL, free);
            map_set_hash_function(map_country_diseases, hash_string);
            Set set = map_node_value(map_countries_dates, node);
            for(SetNode node_set = set_first(set); node_set != SET_EOF; node_set = set_next(set, node_set)){
                Record record = set_node_value(set, node_set);
                map_insert(map_country_diseases, record->disease, create_int(0));
            }

            // Έπειτα ενημερώνουμε το πλήθος εγγραφών για κάθε ασθένεια
            for(SetNode node_set = set_first(set); node_set != SET_EOF; node_set = set_next(set, node_set)){
                Record record = set_node_value(set, node_set);
                Pointer value = map_find(map_country_diseases, record->disease);
                (*(int*)value)++;
            }

            // Εισάγουμε τα στοιχεία σε μια Priority Queue, στην οποία ταξινομούνται με βάση τα values του Map
            PriorityQueue pqueue = pqueue_create(compare_diseases_counter, NULL, NULL);
            for(MapNode node_map = map_first(map_country_diseases); node_map != MAP_EOF; node_map = map_next(map_country_diseases, node_map)){
                pqueue_insert(pqueue, map_node_key(map_country_diseases, node_map));
            }

            // Δημιουργούμε μια λίστα και εισάγουμε τα στοιχεία της Priority Queue
            List list = list_create(NULL);
            while(pqueue_size(pqueue) && k--){
                list_insert_next(list, list_last(list), pqueue_max(pqueue));
                pqueue_remove_max(pqueue);
            }

            pqueue_destroy(pqueue);
            map_destroy(map_country_diseases);

            return list;
        }

    // Αν country == NULL χρησιμοποιούμε το map_NULL_diseases και εισάγουμε τα στοιχεία του σε μια Priority Queue
    // κι έπειτα στη λίστα
    }else if(!country){

        List list = list_create(NULL);

        PriorityQueue pqueue = pqueue_create(compare_NULL_diseases, NULL, NULL);
        for(MapNode node_map = map_first(map_NULL_diseases); node_map != MAP_EOF; node_map = map_next(map_NULL_diseases, node_map)){
            pqueue_insert(pqueue, map_node_key(map_NULL_diseases, node_map));
        }

        while(pqueue_size(pqueue) && k--){
            list_insert_next(list, list_last(list), pqueue_max(pqueue));
            pqueue_remove_max(pqueue);
        }

        pqueue_destroy(pqueue);

        return list;
    }
    
    return NULL;
}