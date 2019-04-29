	TTL selection
	AREA ODEV ,Code , READONLY
	ENTRY
	
	LDR r0,=dizi 
	MOV r1,#0 
dongu1
	CMP r1,#9 
	BEQ son 
	MOV r2,r1 
	ADD r3,r1,#1 
dongu2
	CMP r3,#9 
	BEQ bitis2 
	LDR r4,[r0,r3,LSL #2] 
	LDR r5,[r0,r2,LSL #2] 
	CMP r4,r5
	BGT bitis1 
	MOV r2,r3 
	B bitis1
bitis1
	ADD r3,r3,#1 
	B dongu2
bitis2
	MOV r6,r5 
	LDR r7,[r0,r1,LSL #2] 
	STR r7,[r0,r2,LSL #2]
	STR r6,[r0,r1,LSL #2]
	B bitis3
bitis3
	add r1,r1,#1 
	B dongu1
son
array 
	DCD 0,1,571,621,632,1071,1176,1299,1453
end