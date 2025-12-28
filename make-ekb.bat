copy bd32.cfg cfg.sav
copy bd32lpt1.cfg bd32.cfg
del bd32v1-%1.zip
pkzip bd32v1-%1 bd32.exe bd32.cfg
pkunzip -t bd32v1-%1
copy cfg.sav bd32.cfg
