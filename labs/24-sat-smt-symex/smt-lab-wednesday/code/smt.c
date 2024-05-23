#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include "smt.h"

// Keep track of clauses to be sent to the SAT solver
struct clause
{
    int *literals, n_literals;
};
static struct clause *CLAUSES = NULL;
static int N_CLAUSES = 0;

void clause_arr(int *literals)
{
    int i = N_CLAUSES;
    APPEND_GLOBAL(CLAUSES) = (struct clause){
        .literals = NULL,
        .n_literals = 0,
    };
    for (; *literals; literals++)
    {
        APPEND_FIELD(CLAUSES[i], literals) = *literals;
    }
}

// Keep track of arrays in a tree structure. (store a k v) creates a new array
// whose parent is a, bv_store_key is k, and bv_store_value is v. Whenever a
// get is done on the array, a corresponding bv_lookup is appended to its list.
struct bv_lookup
{
    int bv_key, bv_value;
};

struct array
{
    int array_parent;

    int bv_store_key, bv_store_value;

    struct bv_lookup *bv_lookups;
    int n_bv_lookups;
};
static struct array *ARRAYS = NULL;
static int N_ARRAYS = 0;

// Keep track of bitvectors
struct bv
{
    int *bits;
    int n_bits;
};
static struct bv *BVS = NULL;
static int N_BVS = 0;

// You can get a fresh SAT variable like NEXT_SAT_VAR++
static int NEXT_SAT_VAR = 1;

int new_array()
{
    // Create a new array. It has no parent (-1) and no lookups yet (NULL, 0).
    // Returns its index in the ARRAYS vector.
    // assert(!"Implement me!");
    APPEND_GLOBAL(ARRAYS) = (struct array){
        .array_parent = -1,
        .bv_store_key = -1,
        .bv_store_value = -1,
        .bv_lookups = NULL,
        .n_bv_lookups = 0,
    };
    return N_ARRAYS - 1;
}

int array_store(int old_array, int bv_key, int bv_value)
{
    // Construct a new array that is the same as old_array except bv_key is
    // updated to bv_value. This should look like new_array() except you set
    // array_parent, bv_store_key, bv_store_value correctly.
    // assert(!"Implement me!");
    APPEND_GLOBAL(ARRAYS) = (struct array){
        .array_parent = old_array,
        .bv_store_key = bv_key,
        .bv_store_value = bv_value,
        .bv_lookups = NULL,
        .n_bv_lookups = 0,
    };
    return N_ARRAYS - 1;
}

int array_get(int array, int bv_key, int out_width)
{
    // Create a new bitvector and (on the corresponding array) record this
    // key/value pair lookup.
    // assert(!"Implement me!");
    int bv = new_bv(out_width);
    struct bv_lookup lup = {.bv_key = bv_key, .bv_value = bv};
    APPEND_FIELD(ARRAYS[array], bv_lookups) = lup;

    return bv;
}

int new_bv(int width)
{
    // Create a fresh bitvector of the given width. Note here you need to
    // append to the BVS vector as well as initialize all its bits to fresh SAT
    // variables.
    // assert(!"Implement me!");
    struct bv bit_vector = {.bits = malloc(width * sizeof(int)), .n_bits = width};

    for (int i = 0; i < width; i++)
    {
        bit_vector.bits[i] = NEXT_SAT_VAR++;
    }
    APPEND_GLOBAL(BVS) = bit_vector;

    return N_BVS - 1;
}

int const_bv(int64_t value, int width)
{
    // Like new_bv, except also add clauses asserting its bits are the same as
    // those of @value. Please do little-endian; bits[0] should be value & 1.
    // assert(!"Implement me!");
    struct bv bit_vector = {.bits = malloc(width * sizeof(int)), .n_bits = width};
    APPEND_GLOBAL(BVS) = bit_vector;

    for (int i = 0; i < width; i++)
    {
        int var = NEXT_SAT_VAR++;
        int literal = (value >> i) & 1 ? var : -var;
        bit_vector.bits[i] = var;
        clause(literal);
    }

    return N_BVS - 1;
}

int bv_eq(int bv_1, int bv_2)
{
    int width = BVS[bv_1].n_bits;
    assert(width == BVS[bv_2].n_bits);

    struct bv bvs_1 = BVS[bv_1];
    struct bv bvs_2 = BVS[bv_2];

    // This one is a doozy: add a fresh SAT variable and enough SAT clauses to
    // assert that this variable is true iff all the bits of bv_1 and bv_2 are
    // equal. I suggest using as many of intermediate/temp SAT variables as you
    // need; generally, BCP does a great job at handling those so they're more
    // or less "free" (at least, for our purposes rn!).
    // assert(!"Implement me!");

    int ret = NEXT_SAT_VAR + width;

    for (int i = 0; i < width; i++)
    {
        int cst_var = NEXT_SAT_VAR + i;
        clause_arr((int[]){-cst_var, -bvs_1.bits[i], bvs_2.bits[i], 0});
        clause_arr((int[]){-cst_var, bvs_1.bits[i], -bvs_2.bits[i], 0});
        clause_arr((int[]){cst_var, bvs_1.bits[i], bvs_2.bits[i], 0});
        clause_arr((int[]){cst_var, -bvs_1.bits[i], -bvs_2.bits[i], 0});
        clause_arr((int[]){-ret, cst_var, 0});
    }

    int *cl = malloc((width + 2) * sizeof(int));

    for (int i = 0; i < width; i++)
    {
        int cst_var = NEXT_SAT_VAR++;
        cl[i] = -cst_var;
    }

    cl[width] = ret;
    cl[width + 1] = 0;

    clause_arr(cl);
    NEXT_SAT_VAR++;

    return ret;
}

inline static is_positive(int x)
{
    return x > 0 ? 1 : 0;
}

inline static void triple_and_impl(int x, int y, int c, int z)
{
    // (x and y and c) => z
    clause_arr((int[]){-x, -y, -c, z, 0});
}

inline static void carry_adder(int x, int y, int c0, int z, int c1)
{

    triple_and_impl(x, y, c0, (is_positive(x) + is_positive(y) + is_positive(c0)) % 2 ? z : -z);
    triple_and_impl(x, y, c0, (is_positive(x) + is_positive(y) + is_positive(c0)) / 2 ? c1 : -c1);
}

int bv_add(int bv_1, int bv_2)
{
    int width = BVS[bv_1].n_bits;
    assert(width == BVS[bv_2].n_bits);

    int out = new_bv(width);
    int carry = new_bv(width + 1);

    struct bv bvs_1 = BVS[bv_1];
    struct bv bvs_2 = BVS[bv_2];
    struct bv bvs_out = BVS[out];
    struct bv bvs_carry = BVS[carry];

    // Set the first carry bit to zero
    clause(-bvs_carry.bits[0]);

    // This one is similarly big; basically you want to do a ripple-carry
    // adder to assign the bits of @out based on the bits of @bv_1, @bv_2. I
    // would recommend just trying to encode it as a truth table.
    // assert(!"Implement me!");
    for (int i = 0; i < width; i++)
    {
        for (int j = 0; j < 2; j++)
        {
            for (int k = 0; k < 2; k++)
            {
                for (int l = 0; l < 2; l++)
                {
                    carry_adder((j > 0 ? 1 : -1) * bvs_1.bits[i],
                                (k > 0 ? 1 : -1) * bvs_2.bits[i],
                                (l > 0 ? 1 : -1) * bvs_carry.bits[i],
                                bvs_out.bits[i],
                                bvs_carry.bits[i + 1]);
                }
            }
        }
    }
    return out;
}

static void array_axioms(struct array array, int compare_up_to,
                         struct bv_lookup bv_lookup, int already_handled)
{

    if (array.array_parent >= 0)
    {
        // This array is of the form (store parent k v), where k and v are
        // array.bv_store_*. Add clauses here of the form:
        // ((k == lookup.key) ^ !already_handled) => (store_val == bv_val)
        // Hint: use bv_eq!
        // assert(!"Implement me!");
        int k_eq_lookup = bv_eq(array.bv_store_key, bv_lookup.bv_key);
        int val_eq = bv_eq(array.bv_store_value, bv_lookup.bv_value);
        clause_arr((int[]){-k_eq_lookup, already_handled, val_eq, 0});

        // Now we need to record if that worked or not. If this is picking up
        // that store, then we don't need the value to match with any prior
        // values (because they were overridden). So make a fresh SAT variable
        // new_already_handled and assign it:
        // (already_handled or (k == lookup.key)) <=> new_already_handled
        // assert(!"Implement me!");
        // int new_already_handled = new_bv(BVS[already_handled].n_bits);
        int new_already_handled = NEXT_SAT_VAR++;

        clause_arr((int[]){-already_handled, new_already_handled, 0});
        clause_arr((int[]){-k_eq_lookup, new_already_handled, 0});

        clause_arr((int[]){-new_already_handled, already_handled, k_eq_lookup, 0});

        // Then update already_handled = new_already_handled
        // assert(!"Implement me!");
        already_handled = new_already_handled;
    }

    // return;

    // If we haven't found a set yet, compare lookup[key] to all prior lookups
    // on the array. If keys eq, vals should be eq too.
    for (int i = 0; i < compare_up_to; i++)
    {
        struct bv_lookup sub_lookup = array.bv_lookups[i];
        // We want:
        // !already_handled ^ (sub_lookup.key == bv_lookup.key)
        // => sub_lookup.value == bv_lookup.value

        // assert(!"Implement me!");
        int key_eq = bv_eq(sub_lookup.bv_key, bv_lookup.bv_key);
        int val_eq = bv_eq(sub_lookup.bv_value, bv_lookup.bv_value);
        clause_arr((int[]){already_handled, -key_eq, val_eq});
    }

    // If we haven't found a set yet, go back through parents and repeat this
    // reasoning.
    if (array.array_parent >= 0)
    {
        array_axioms(ARRAYS[array.array_parent],
                     ARRAYS[array.array_parent].n_bv_lookups,
                     bv_lookup, already_handled);
    }
}

int *SAT_SOLUTION = NULL;
int solve()
{
    assert(!SAT_SOLUTION);
    char constraint_path[256] = "";
    sprintf(constraint_path, "temp_files/constraints.%d.dimacs", getpid());

    FILE *fout = fopen(constraint_path, "w");

    int zero = NEXT_SAT_VAR++;
    clause(-zero);

    // Go through array stores & lookups and set up the N^2 bv_eq implications
    // implied
    for (int i = 0; i < N_ARRAYS; i++)
    {
        for (int j = 0; j < ARRAYS[i].n_bv_lookups; j++)
        {
            array_axioms(ARRAYS[i], j, ARRAYS[i].bv_lookups[j], zero);
        }
    }

    // Print the clauses and some info about the variable interpretations
    for (int i = 0; i < N_BVS; i++)
    {
        fprintf(fout, "c bitvector %d:", i);
        for (int j = 0; j < BVS[i].n_bits; j++)
            fprintf(fout, " %d", BVS[i].bits[j]);
        fprintf(fout, "\n");
    }
    fprintf(fout, "p cnf %d %d\n", NEXT_SAT_VAR - 1, N_CLAUSES);

    // Iterate through the clauses and print them in DIMACS format.
    // assert(!"Implement me!");
    for (int i = 0; i < N_CLAUSES; i++)
    {
        for (int j = 0; j < CLAUSES[i].n_literals; j++)
        {
            fprintf(fout, "%d ", CLAUSES[i].literals[j]);
        }
        fprintf(fout, "0\n");
    }

    fclose(fout);

    char result_path[256] = "";
    sprintf(result_path, "temp_files/results.%d.txt", getpid());

    char cmd[1024] = "";
#ifdef USE_MINISAT
    sprintf(cmd, "minisat %s %s > /dev/null", constraint_path, result_path);
#else
    sprintf(cmd, "cat %s | ./fast > %s", constraint_path, result_path);
#endif
    assert(system(cmd) != -1);

    FILE *results = fopen(result_path, "r");
    char sat_or_unsat[10] = "NONE";
    assert(fscanf(results, "%s ", sat_or_unsat));
    if (!strcmp(sat_or_unsat, "UNSAT"))
    {
        fclose(results);
        return 0;
    }
    else if (!strcmp(sat_or_unsat, "SAT"))
    {
        SAT_SOLUTION = malloc(NEXT_SAT_VAR * sizeof(SAT_SOLUTION[0]));
        int literal = 0;
        while (!feof(results) && fscanf(results, " %d ", &literal))
        {
            if (literal < 0)
                SAT_SOLUTION[-literal] = 0;
            else
                SAT_SOLUTION[literal] = 1;
        }
        fclose(results);
        return 1;
    }
    exit(1);
}

int64_t get_solution(int bv, int as_signed)
{
    assert(SAT_SOLUTION);

    // Read the bits for @bv from SAT_SOLUTION into an int64_t.
    // assert(!"Implement me!");

    int64_t ret = 0;
    for (int i = 0; i < BVS[bv].n_bits; i++)
    {
        ret |= (SAT_SOLUTION[BVS[bv].bits[i]] << i);
    }

    return ret;
}
