#include <gtk/gtk.h>
#include <r_core.h>
#include "util.h"

typedef struct rg_disas_list{
    GHashTable *entries_ht;
    GList *entries_list;
} RGDisasList;

typedef struct rg_xref_entry_t{
    const char *type;
    ut64 address;
    char *funname;
} RGXrefEntry;

typedef struct rg_disas_entry_t{
    ut64 offset;
    short oplen;
    char *type;
    long jump;
    long fail;
    RAsmOp asmop;
    RAnalOp analop;
    GList *xrefs;
} RGDisasEntry;

typedef struct rg_disas_cursor_t{
    ut64 offset;
} RGDisasCursor;

void rg_get_xrefs_at_offset(RCore *r, ut64 offset, RGDisasEntry *entry);

int rg_disas_buf(RCore *r, ut64 addr, ut8 *buf, RGDisasList *disas_list, int len) ;

void new_pd_list(RCore *r, RGDisasCursor *cursor);
void new_pda_list(RCore *r, RGDisasCursor *cursor);
void new_pdb_list(RCore *r);
void new_pdr_list(RCore *r);
void new_pdf_list(RCore *r);
void new_pdi_list(RCore *r, ut64 *offset, ut64 len);
void new_pds_list(RCore *r);
