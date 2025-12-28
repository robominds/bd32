del *.bak
copy bd32.cfg cfg.sav
copy bd32lpt1.cfg bd32.cfg
copy bdm-if.zip bd32v%1.zip
pkzip bd32v%1 bd32.exe bd32.doc bd32.cfg read.me ipd.inc test.*
pkunzip -t bd32v%1
copy cfg.sav bd32.cfg
