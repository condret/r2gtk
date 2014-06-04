r2gtk
=====

radare2 gtk frontend

Currently I am working on the disassembly view / code navigation features.
I'll perform code organization when I get the main features of the disas view completed.

TODO (disas_view):
------------------
* When hovering over an xref, show a preview of the contents at that address how IDA Pro does it.
* Implement right click functionality for each line
* Possibly use gtk source view
* Impement status bar / info pane for currently highlighted line
* Obviously formattting of the disassembly
* Once the base disas view features are completed (or close to it), the main interface will be built.
The tab system will be created, disassembly views can share buffers, or contain new disassemblies.
The menu system will allow the user to create new disassembly views like pdf, pda, pdb, pd, pdi (starting from current cursor position, or input a starting position), pds, yada yada


make disas_view
./disas_view /bin/ls

