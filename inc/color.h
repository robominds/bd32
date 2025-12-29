/* color.h - define color attributes */

#pragma once

#define	COMMANDOUTPUTCOLOR	(Attributes [0])
#define	COMMANDINPUTCOLOR	(Attributes [1])
#define	PROMPTCOLOR		(Attributes [2])
#define	REGISTERDISPLAYCOLOR	(Attributes [3])
#define	BREAKPOINTCOLOR		(Attributes [4])
#define	PCCOLOR			(Attributes [5])
#define	STATUSLINECOLOR		(Attributes [6])
#define	STATUSITEMCOLOR		(Attributes [7])
#define	BORDERCOLOR		(Attributes [8])
#define	ATTRIBUTECOUNT		9

extern unsigned char Attributes [ATTRIBUTECOUNT];

/* end of color.h */
