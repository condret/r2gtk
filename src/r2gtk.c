#include <r_core.h>
#include "disas_view.h"


static struct r_core_t r;

int main(int argc, char **argv, char **envp){
    gtk_init (&argc, &argv);
    ut64 baddr = 0LL;
    RCoreFile *fh = NULL;
    char *pfile = NULL, *filepath=NULL;
    int perms = R_IO_READ;
    ut64 mapaddr = 0LL;
    const char *asmarch = NULL;
	const char *asmos = NULL;
	const char *asmbits = NULL;
	short fullfile = 1, quiet=0, do_analysis=1;
	
	r_sys_set_environ (envp);
	
    r_core_init (&r);

	r_core_loadlibs (&r, R_CORE_LOADLIBS_ALL, NULL);
    r_config_set_i (r.config, "bin.baddr", baddr);
        
    pfile = argv[1];
	fh = r_core_file_open (&r, pfile, perms, mapaddr);
	if (perms & R_IO_WRITE) {
		if (!fh) {
			r_io_create (r.io, pfile, 0644, 0);
			fh = r_core_file_open (&r, pfile, perms, mapaddr);
		}
	}
    if (fh == NULL) {
		if (pfile && *pfile) {
			if (perms & R_IO_WRITE)
				eprintf ("Cannot open '%s' for writing.\n", pfile);
			else eprintf ("Cannot open '%s'\n", pfile);
		} else eprintf ("Missing file to open\n");
		return 1;
	}
	if (r.file == NULL){ // no given file
		return 1;
    }
    if (r.file && r.file->filename){
		filepath = r.file->filename;
    }
	if (!r_core_bin_load (&r, filepath, baddr)){
		r_config_set (r.config, "io.va", "false");
	}
    if (asmarch) r_config_set (r.config, "asm.arch", asmarch);
	if (asmbits) r_config_set (r.config, "asm.bits", asmbits);
	if (asmos) r_config_set (r.config, "asm.os", asmos);
	r_core_bin_update_arch_bits (&r);
	if (fullfile) r_core_block_size (&r, r.file->size);
	r_core_seek (&r, r.offset, 1);
	
	const char *global_rc = R2_PREFIX"/share/radare2/radare2rc";
	if (r_file_exists (global_rc)){
		(void)r_core_run_script (&r, global_rc);
	}
	
	char f[128];
	snprintf(f, sizeof(f), "%s.r2", pfile);
	if (r_file_exists (f)) {
		if (!quiet)
			eprintf ("NOTE: Loading '%s' script.\n", f);
		r_core_cmd_file (&r, f);
	}
	
	if (do_analysis) {
		r_core_cmd0 (&r, "aa");
		r_cons_flush ();
	}
	
	r.num->value = 0;
	
    RGDisasCursor *cursor=g_new(RGDisasCursor, 1);
    cursor->offset = r.offset;
    new_pd_list(&r, cursor);
    
    return 0;
}
