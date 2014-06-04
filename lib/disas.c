#include "disas.h"

void rg_get_xrefs_at_offset(RCore *r, ut64 offset, RGDisasEntry *entry){
	RList *xrefs;
	RAnalRef *refi;
	RListIter *iter;
    if((xrefs = r_anal_xref_get(r->anal, offset))){
        r_list_foreach(xrefs, iter, refi){
            if(refi->at == offset){
                RAnalFunction *fun = r_anal_fcn_find (
			                            r->anal, refi->addr,
			                            R_ANAL_FCN_TYPE_FCN |
			                            R_ANAL_FCN_TYPE_ROOT);
                
                const char *_xref_type = "UNKNOWN";
                switch(refi->type){
                    case R_ANAL_REF_TYPE_NULL:
                        _xref_type = "UNKNOWN";
                    break;
                    case R_ANAL_REF_TYPE_CODE:
                        _xref_type = "JMP";
                    break;
                    case R_ANAL_REF_TYPE_CALL:
                        _xref_type = "CALL";
                    break;
                    case R_ANAL_REF_TYPE_DATA:
                        _xref_type = "DATA";
                    break;
                    case R_ANAL_REF_TYPE_STRING:
                        _xref_type = "STRING";
                    break;
                }
                entry->xrefs = NULL;
                RGXrefEntry *xref_entry = g_new(RGXrefEntry, 1);
                xref_entry->type = _xref_type;
                xref_entry->address = refi->addr;
                xref_entry->funname = fun ? fun->name : "unk";
                
                entry->xrefs = g_list_prepend(entry->xrefs, xref_entry);
            }
        }
        entry->xrefs = g_list_reverse(entry->xrefs);
    }
}

int rg_disas_buf(RCore *r, ut64 addr, ut8 *buf, RGDisasList *disas_list, int len) {
	RAsmOp asmop;
	RAnalOp analop;
	int i, oplen, ret;
    
    
	if (r->anal && r->anal->cur && r->anal->cur->reset_counter	) {
		r->anal->cur->reset_counter (r->anal, addr);
	}
	for (i=0; i<len;) {
        RGDisasEntry *entry = g_new(RGDisasEntry, 1);
                
		ut64 offset = addr +i;

		r_asm_set_pc (r->assembler, offset);
		ret = r_asm_disassemble (r->assembler, &asmop, buf+i, len-i+5);
        
        entry->offset = offset;
        entry->asmop = asmop;
        
	    if(ret<1){
	        entry->type = "invalid";
	        entry->oplen = 1;
	        i++;
	        continue;
	    }
		r_anal_op (r->anal, &analop, offset, buf+i, len-i+5);

		oplen = r_asm_op_get_size (&asmop);
        
        entry->oplen = oplen;
        entry->analop = analop;
        entry->type = r_anal_optype_to_string (analop.type);
        
        if (analop.jump != UT64_MAX) {
			entry->jump = analop.jump;
			if (analop.fail != UT64_MAX)
				entry->fail = analop.fail;
		}
		
		entry->xrefs = NULL;
		rg_get_xrefs_at_offset(r, offset, entry);
		//g_list_foreach(entry->xrefs, (GFunc)print_xrefs, NULL);
		disas_list->entries_list = g_list_prepend(disas_list->entries_list, entry);
		g_hash_table_insert(disas_list->entries_ht, GINT_TO_POINTER(entry->offset), entry);
		i += oplen;
	}
	disas_list->entries_list = g_list_reverse(disas_list->entries_list);
	return R_TRUE;
}

