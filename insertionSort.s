    TTL insertion
	AREA odev,Code , READWRITE
	ENTRY
	
	LDR r0,=array 
	MOV r1,#1 
dongu1
	CMP r1,#9 
	BGT son 
	MOV r2,r1 
dongu2
	CMP r2,#0 ;
	BEQ bitis2 
	LDR r3,[r0,r2,LSL #2] 
	SUB r4,r2,#1
	LDR r5,[r0,r4,LSL #2]
	CMP r3,r5 
	BGT bitis  
	MOV r6,r3 
	STR r5,[r0,r2,LSL #2]
	STR r6,[r0,r4,LSL #2]
bitis1
	SUB r2,r2,#1
	B dongu2
bitis2
	ADD r1,r1,#1
	B dongu1
son

	
array DCD 0,1,571,621,632,1071,1176,1299,1453

end