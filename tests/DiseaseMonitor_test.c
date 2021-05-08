//////////////////////////////////////////////////////////////////
//
// Unit tests for DiseaseMonitor.h
// Οποιαδήποτε υλοποίηση οφείλει να περνάει όλα τα tests.
//
//////////////////////////////////////////////////////////////////

#include "acutest.h"			// Απλή βιβλιοθήκη για unit testing
#include <limits.h>

#include "DiseaseMonitor.h"

// test records
struct record records[] = {
	// country = house, diseases might be inaccurate
	{ .id = 1,  .name = "Daenerys", .country = "Targaryen", .disease = "Grayscale", .date = "0301-01-01" },
	{ .id = 2,  .name = "Jaime",    .country = "Lannister", .disease = "Grayscale", .date = "0302-01-01" },
	{ .id = 3,  .name = "Cersei",   .country = "Lannister", .disease = "Grayscale", .date = "0302-01-01" },
	{ .id = 4,  .name = "Ned",      .country = "Stark",     .disease = "Headache",  .date = "0298-01-01" },
	{ .id = 5,  .name = "Rhaegar",  .country = "Targaryen", .disease = "Pale Mare", .date = "0281-01-01" },
	{ .id = 6,  .name = "Sansa",    .country = "Stark",     .disease = "Pale Mare", .date = "0301-01-01" },
	{ .id = 7,  .name = "Aerys II", .country = "Targaryen", .disease = "Madness",   .date = "0271-01-01" },
	{ .id = 8,  .name = "Tyrion",   .country = "Lannister", .disease = "Grayscale", .date = "0299-01-01" },
	{ .id = 9,  .name = "Tywin",    .country = "Lannister", .disease = "Grayscale", .date = "0301-01-01" },
	{ .id = 10, .name = "Arya",     .country = "Stark",     .disease = "Pale Mare", .date = "0301-01-01" },
	{ .id = 11, .name = "Bran",     .country = "Stark",     .disease = "Pale Mare", .date = "0301-01-01" },
	{ .id = 12, .name = "Sandor",   .country = "Clegane",   .disease = "Burns",     .date = "0299-01-01" },
	{ .id = 13, .name = "Aegon",    .country = "Targaryen", .disease = "Grayscale", .date = "0296-01-01" },
	{ .id = 14, .name = "Joffrey",  .country = "Baratheon", .disease = "Grayscale", .date = "0301-01-01" },
	{ .id = 15, .name = "Petyr",    .country = "Baelish",   .disease = "Grayscale", .date = "0300-01-01" },
	{ .id = 16, .name = "Euron",    .country = "Greyjoy",   .disease = "Pale Mare", .date = "0300-01-01" },
	{ .id = 17, .name = "Theon",    .country = "Greyjoy",   .disease = "Pale Mare", .date = "0300-01-01" },
	{ .id = 18, .name = "Oberyn",   .country = "Martell",   .disease = "Eye pain",  .date = "0301-01-01" },
	{ .id = 19, .name = "Robert",   .country = "Baratheon", .disease = "Grayscale", .date = "0298-01-01" },
	{ .id = 20, .name = "Jorah",    .country = "Mormont",   .disease = "Grayscale", .date = "0299-01-01" },

};
int record_no = sizeof(records)/sizeof(struct record);


// Ελέγχει ότι οι εγγραφές στη λίστα result περιέχουν τα ids στον πινακα ids.
// Η σειρά ΔΕΝ έχει σημασία.
void check_record_list(List result, int ids[], int size) {
	bool found[size];
	for (int i = 0; i < size; i++)
		found[i] = false;

	TEST_ASSERT(list_size(result) == size);

	for (ListNode node = list_first(result); node != LIST_EOF; node = list_next(result, node)) {
		int id = ((Record)list_node_value(result, node))->id;

		int pos;
		for (pos = 0; pos < size && ids[pos] != id; pos++)
			;
		TEST_ASSERT(pos < size && !found[pos]);

		found[pos] = true;
	}

	list_destroy(result);
}


void test_init(void) {
	dm_init();

	List list = dm_get_records(NULL, NULL, NULL, NULL);
	TEST_ASSERT(list_size(list) == 0);
	list_destroy(list);

	dm_destroy();
}

void test_insert(void) {
	dm_init();

	int ids[record_no];

	for(int i = 0; i < record_no; i++) {
		ids[i] = records[i].id;

		// Εισαγωγή της εγγραφής i
		TEST_ASSERT(!dm_insert_record(&records[i]));	
		TEST_ASSERT( dm_insert_record(&records[i]));		// replace, returns true

		// Ελεγχος ότι το monitor περιέχει όλες τις εγγραφές μέχρι και το i
		List list = dm_get_records(NULL, NULL, NULL, NULL);
		check_record_list(list, ids, i+1);
	}

	dm_destroy();
}

void test_remove(void) {
	dm_init();

	// insert records
	int ids[record_no];
	for(int i = 0; i < record_no; i++) {
		ids[i] = records[i].id;
		dm_insert_record(&records[i]);	
	}

	for(int i = 0; i < record_no; i++) {
		// Delete της εγγραφής i
		TEST_ASSERT(dm_remove_record(records[i].id));	
		TEST_ASSERT(!dm_remove_record(records[i].id));		// already deleted, returns false

		// Ελεγχος ότι το monitor περιέχει όλες τις εγγραφές από το i μέχρι το τέλος
		List list = dm_get_records(NULL, NULL, NULL, NULL);
		check_record_list(list, &ids[i+1], record_no-i-1);
	}

	dm_destroy();
}

void test_get_records(void) {
	dm_init();

	// insert records
	for(int i = 0; i < record_no; i++)
		dm_insert_record(&records[i]);	

	// try various search criteria
	List list1 = dm_get_records("Pale Mare", NULL, NULL, NULL);
	int ids1[] = {5, 6, 10, 11, 16, 17};
	check_record_list(list1, ids1, sizeof(ids1)/sizeof(int));

	List list2 = dm_get_records(NULL, "Targaryen", NULL, NULL);
	int ids2[] = {1, 5, 7, 13};
	check_record_list(list2, ids2, sizeof(ids2)/sizeof(int));

	List list3 = dm_get_records("Pale Mare", "Targaryen", NULL, NULL);
	int ids3[] = {5};
	check_record_list(list3, ids3, sizeof(ids3)/sizeof(int));

	List list4 = dm_get_records(NULL, NULL, "0301-01-01", NULL);
	int ids4[] = {1, 2, 3, 6, 9, 10, 11, 14, 18};
	check_record_list(list4, ids4, sizeof(ids4)/sizeof(int));

	List list5 = dm_get_records(NULL, NULL, NULL, "0297-01-01");
	int ids5[] = {5, 7, 13};
	check_record_list(list5, ids5, sizeof(ids5)/sizeof(int));

	List list6 = dm_get_records("Grayscale", NULL, "0299-01-01", "0300-01-01");
	int ids6[] = {8, 15, 20};
	check_record_list(list6, ids6, sizeof(ids6)/sizeof(int));

	List list7 = dm_get_records(NULL, "Lannister", "0299-01-01", "0301-01-01");
	int ids7[] = {8, 9};
	check_record_list(list7, ids7, sizeof(ids7)/sizeof(int));

	dm_destroy();
}

// Εκτελεί τη dm_count_records και ελέγχει ότι επιστρέφει τον ίδιο αριθμό
// αποτελεσμάτων με την dm_get_records.
void count_and_test(String disease, String country, Date date_from, Date date_to) {
	List recs = dm_get_records(disease, country, date_from, date_to);
	int count = list_size(recs);
	list_destroy(recs);

	TEST_ASSERT(dm_count_records(disease, country, date_from, date_to) == count);
}

void test_count_records(void) {
	dm_init();

	// insert records
	for(int i = 0; i < record_no; i++)
		dm_insert_record(&records[i]);	

	// try various search criteria
	count_and_test("Pale Mare", NULL,        NULL,         NULL        );
	count_and_test(NULL,        "Targaryen", NULL,         NULL        );
	count_and_test("Pale Mare", "Targaryen", NULL,         NULL        );
	count_and_test(NULL,        NULL,        "0301-01-01", NULL        );
	count_and_test(NULL,        NULL,        NULL,         "0297-01-01");
	count_and_test("Grayscale", NULL,        "0299-01-01", "0300-01-01");
	count_and_test(NULL,        "Lannister", "0299-01-01", "0301-01-01");

	dm_destroy();
}

// Ελεγχος ότι κάθε ασθένεια έχει λιγότερες εγγραφές από την προηγούμενη
void run_and_test_top_diseases(int k, String country) {
	List result = dm_top_diseases(k, country);
	int last_count = INT_MAX;

	TEST_ASSERT(list_size(result) == k);

	for (ListNode node = list_first(result); node != LIST_EOF; node = list_next(result, node)) {
		String disease = list_node_value(result, node);
		int count = dm_count_records(disease, country, NULL, NULL);
		TEST_ASSERT(count <= last_count);
		last_count = count;
	}

	list_destroy(result);
}

void test_top_diseases(void) {
	dm_init();

	// insert records
	for(int i = 0; i < record_no; i++)
		dm_insert_record(&records[i]);	

	// try various search criteria
	for (int k = 1; k <= 2; k++)
		run_and_test_top_diseases(k, "Stark");

	for (int k = 1; k <= 3; k++)
		run_and_test_top_diseases(k, "Targaryen");

	for (int k = 1; k <= 6; k++)
		run_and_test_top_diseases(k, NULL);

	dm_destroy();
}



// Λίστα με όλα τα tests προς εκτέλεση
TEST_LIST = {
	{ "dm_init", test_init },
	{ "dm_insert_record", test_insert },
	{ "dm_remove_record", test_remove },
	{ "dm_get_records", test_get_records },
	{ "dm_count_records", test_count_records },
	{ "dm_top_diseases", test_top_diseases },

	{ NULL, NULL } // τερματίζουμε τη λίστα με NULL
}; 