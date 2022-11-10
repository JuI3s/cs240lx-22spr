/*************************************************************************
 * engler, cs240lx: Purify/Boehm style leak checker/gc starter code.
 *
 * We've made a bunch of simplifications.  Given lab constraints:
 * people talk about trading space for time or vs. but the real trick 
 * is how to trade space and time for less IQ needed to see something 
 * is correct. (I haven't really done a good job of that here, but
 * it's a goal.)
 *
 */
#include "rpi.h"
#include "rpi-constants.h"
#include "ckalloc.h"
#include "kr-malloc.h"

static void * heap_start;
static void * heap_end;
static int init_p;
void *sbrk(long increment) {
	trace("in sbrk\n");
    assert(increment > 0);
    if(!init_p) {
        unsigned onemb = 0x100000;
        heap_start = (void*)onemb;
        heap_end = (char*)heap_start + onemb;
        kmalloc_init_set_start(onemb, onemb);
        init_p = 1;
    }
    return kmalloc(increment);
}

// quick check that the pointer is between the start of
// the heap and the last allocated heap pointer.  saves us 
// walk through all heap blocks.
//
// we could warn if the pointer is within some amount of slop
// so that you can detect some simple overruns?
static int in_heap(void *p) {
    // should be the last allocated byte(?)
    if(p < heap_start || p >= heap_end)
        return 0;
    // output("ptr %p is in heap!\n", p);
    return 1;
}

// given potential address <addr>, returns:
//  - 0 if <addr> does not correspond to an address range of 
//    any allocated block.
//  - the associated header otherwise (even if freed: caller should
//    check and decide what to do in that case).
//
// XXX: you'd want to abstract this some so that you can use it with
// other allocators.  our leak/gc isn't really allocator specific.
static hdr_t *is_ptr(uint32_t addr) {
    void *p = (void*)addr;
    
    if(!in_heap(p))
        return 0;
    return ck_ptr_to_hdr(p);
}

// mark phase:
//  - iterate over the words in the range [p,e], marking any block 
//    potentially referenced.
//  - if we mark a block for the first time, recurse over its memory
//    as well.
//
// NOTE: if we have lots of words, could be faster with shadow memory / a lookup
// table.  however, given our small sizes, this stupid search may well be faster :)
// If you switch: measure speedup!
//
#include "libc/helper-macros.h"
static void mark(uint32_t *p, uint32_t *e) {
    assert(p<e);
    // maybe keep this same thing?
    assert(aligned(p,4));
    assert(aligned(e,4));

	const unsigned num_words = e - p;
	for (unsigned i = 0; i < num_words; i++) {
		hdr_t *curr_word = is_ptr(p[i]);
		if (curr_word && !curr_word->mark) {
			// mark since curr_word may be ptr
			curr_word->mark = 1;

			//ptr is in valid block, does it point to head of user block, or  their user data
			if ((void*)curr_word + sizeof(hdr_t) == (void*)p[i]) {
				//p[i] is pointing to start of user block
				curr_word->refs_start++;
			} else if ((void*)curr_word + sizeof(hdr_t) + sizeof(Header) < (void*)p[i] && (void*)p[i] <= (void*)curr_word + sizeof(hdr_t) + sizeof(Header)  + curr_word->nbytes_alloc) {
				// p[i] points to middle of data in user block
				curr_word->refs_middle++;
			}
			// } else if ((void*)p[i] >= (void*)curr_word && (void*)p[i] <= (void*)curr_word + sizeof(hdr_t)) {
			// 	trace("found ptr to header of block.\n");
			// 	curr_word->refs_start++;
			// }
			// recurse on mem block it points to
			uint32_t *block_start = (uint32_t*)((char*)curr_word + sizeof(hdr_t));
			uint32_t *block_end = (uint32_t*)((char*)block_start + curr_word->nbytes_alloc);
			mark(block_start, block_end);
		} 
	}
}


// do a sweep, warning about any leaks.
//
//
static unsigned sweep_leak(int warn_no_start_ref_p) {
	unsigned nblocks = 0, errors = 0, maybe_errors=0;
	output("---------------------------------------------------------\n");
	output("checking for leaks:\n");

	// unimplemented();
	hdr_t *curr = ck_first_hdr();
	while (curr) {
		nblocks++;
		unsigned alloced = curr->state == ALLOCED;
		int no_start = (!(curr->refs_start) && warn_no_start_ref_p);
		if (alloced && curr->mark == 0 ) {
			trace("first: curr = %x, start refs = %d, mid refs = %d\n", curr, curr->refs_start, curr->refs_middle);
			//block is alloc'd, was not marked, and has not been detected as a leak yet
				// curr was alloc'd but not discovered by mark, a textbook leak
			curr->leaked = 1;
			errors++;
		} else if (no_start && alloced) {
			trace("midd: curr = %x, start refs = %d, mid refs = %d\n", curr, curr->refs_start, curr->refs_middle);
				// refs to mid of block, not start, likely a leak
			maybe_errors++;
			// } else if (!curr->refs_start && !curr->refs_middle) {
			// 	//certainly garbage since no refs to them
			// 	errors++;
			// 	curr->leaked = 1;
			// }
		} else if (!warn_no_start_ref_p && curr->refs_middle && !alloced)  {
			//either dont care about no start  ref and there is a middle ref
			// or wedo care about start ref, but we do have a start and middle;
			trace(" last: curr = %x, start refs = %d, mid refs = %d\n", curr, curr->refs_start, curr->refs_middle);
			maybe_errors++;
		}
		trace("outside: curr = %x, start refs = %d, mid refs = %d\n", curr, curr->refs_start, curr->refs_middle);
		curr = ck_next_hdr(curr);
	}


	trace("\tGC:Checked %d blocks.\n", nblocks);
	if(!errors && !maybe_errors)
		trace("\t\tGC:SUCCESS: No leaks found!\n");
	else
		trace("\t\tGC:ERRORS: %d errors, %d maybe_errors\n", 
						errors, maybe_errors);
	output("----------------------------------------------------------\n");
	return errors + maybe_errors;
}

// write the assembly to dump all registers.
// need this to be in a seperate assembly file since gcc 
// seems to be too smart for its own good.
void dump_regs(uint32_t *v, ...);

// a very slow leak checker.
static void mark_all(void) {

    // slow: should not need this: remove after your code
    // works.
    for(hdr_t *h = ck_first_hdr(); h; h = ck_next_hdr(h)) {
        h->mark = h->refs_start = h->refs_middle = 0;
    }
	// pointers can be on the stack, in registers, or in the heap itself.

    // get all the registers.
    uint32_t regs[16];
    dump_regs(regs);

    // kill caller-saved registers
    regs[0] = 0;
    regs[1] = 0;
    regs[2] = 0;
    regs[3] = 0;

    mark(regs, &regs[16]);

    assert(regs[0] == (uint32_t)regs[0]);
    // mark(regs, &regs[14]);


    // mark the stack: we are assuming only a single
    // stack.  note: stack grows down.
    uint32_t *stack_top = (void*)STACK_ADDR;
	mark((uint32_t*)regs[13], stack_top);

    // these symbols are defined in our memmap
    extern uint32_t __bss_start__, __bss_end__;
    mark(&__bss_start__, &__bss_end__);

    extern uint32_t __data_start__, __data_end__;
    mark(&__data_start__, &__data_end__);
}

// return number of bytes allocated?  freed?  leaked?
// how do we check people?
unsigned ck_find_leaks(int warn_no_start_ref_p) {
	trace("Marking...");
    mark_all();
	output("Done!\n");
    return sweep_leak(warn_no_start_ref_p);
}

// used for tests.  just keep it here.
void check_no_leak(void) {
    // when in the original tests, it seemed gcc was messing 
    // around with these checks since it didn't see that 
    // the pointer could escape.
    gcc_mb();
    if(ck_find_leaks(1))
        panic("GC: should have no leaks!\n");
    else
        trace("GC: SUCCESS: no leaks!\n");
    gcc_mb();
}

// used for tests.  just keep it here.
unsigned check_should_leak(void) {
    // when in the original tests, it seemed gcc was messing 
    // around with these checks since it didn't see that 
    // the pointer could escape.
    gcc_mb();
    unsigned nleaks = ck_find_leaks(1);
    if(!nleaks)
        panic("GC: should have leaks!\n");
    else
        trace("GC: SUCCESS: found %d leaks!\n", nleaks);
    gcc_mb();
    return nleaks;
}

// similar to sweep_leak: but mark unreferenced blocks as FREED.
static unsigned sweep_free(void) {
	unsigned nblocks = 0, nfreed=0, nbytes_freed = 0;
	output("---------------------------------------------------------\n");
	output("compacting:\n");

	hdr_t *curr = ck_first_hdr();
	while (curr) {
		nblocks++;
		if (curr->state == ALLOCED && curr->mark != 1) {
			// allocated block was not marked, and not detected as leak before
			curr->state = FREED;
			nfreed++;
			nbytes_freed += curr->nbytes_alloc;
			trace("leak found for block: %d [addr = %d]. Freeing.\n", curr->block_id, curr);
		}
		curr = ck_next_hdr(curr);
	}

	trace("\tGC:Checked %d blocks, freed %d, %d bytes\n", nblocks, nfreed, nbytes_freed);

    return nbytes_freed;
}

unsigned ck_gc(void) {
    mark_all();
    unsigned nbytes = sweep_free();

    // perhaps coalesce these and give back to heap.  will have to modify last.

    return nbytes;
}

void implement_this(void) {
    panic("did not implement dump_regs!\n");
}

