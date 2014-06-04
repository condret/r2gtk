#include <gtk/gtk.h>
#include "util.h"
/*
void print_xrefs(gpointer item){
    RGXrefEntry *xref = (RGXrefEntry *) item;
    
    printf("\nxref_type: %s", xref->type);
    printf(" funname: %s", xref->funname);
    printf(" address: %"PFMT64d, xref->address);
}

void print_list(gpointer item){
    RGDisasEntry *entry = (RGDisasEntry *)item;
    
    g_list_foreach(entry->xrefs, (GFunc)print_xrefs, NULL);
    printf("offset: %"PFMT64d, entry->offset);
    printf(" size: %d", entry->oplen);
    printf(" opcode: %s", entry->asmop.buf_asm);
    printf(" bytes: %s", entry->asmop.buf_hex);
    printf(" type: %s\n", r_anal_optype_to_string (entry->analop.type));
    
}
*/
/*
void init_list(RGList *list, size_t list_size) {
    list->items = (int *)malloc(list_size * sizeof(int));
    list->used = 0;
    list->size = list_size;
}

void list_append(RGList *list, void *item) {
    if (list->used == list->size) {
        list->size *= 2;
        list->items = (int *)realloc(list->items, list->size * sizeof(int));
    }
    list->items[list->used++] = item;
}

void free_list(RGList *list) {
    free(list->items);
    list->items = NULL;
    list->used = list->size = 0;
}
*/

