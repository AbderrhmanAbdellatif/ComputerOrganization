	TTL Bitoperators
	AREA program, CODE, READONLY
	ENTRY
;    ;DIZIDEKI EN BUYUK ELEMAN
;	;DIZININ BALANGIC ADRESI R6
;	;DIZININ BOYUTU R7
;	;BNE ESIT DEGIL , BLE KUCUK ISE 
;	;BHI BUYUK ISE , BILE KUCUK ESIT 
;	LDR R6,=CDIZI 
;	MOV R7,#3
;	MOV R1,#0
;	LDR R3,[R6]
;	
;ILK	ADD R1,R1,#4
;	LDR R5,[R6,R1]
;	CMP R3,R5
;    BHI sonra
;	MOV R3,R5
;sonra subs R7,R7,#1
;      CMP R7,#0
;	  BNE ILK
;CDIZI DCD 2,4,6,1
; LDR R7,=Cdizi
; MOV R0,#0 ; Dizi döngü sayaci
; MOV R1,#0 ;1 lerin sayisi
; MOV R3,#0 ;Dizi offset
;ilk CMP R0,#5
; BEQ EXIT
; ADD R0,R0,#1
; LDR R2,[R7,R3]
; MOV R4,#0 ; binary sayac
;islem
; CMP R4,#32
; BEQ son
; ADD R4,R4,#1
; AND R5,R2,#1
; LSR R2,R2,#1
; CMP R5,#1
; BNE islem
; ADD R1,R1,#1
; B islem
;son
; ADD R3,R3,#4
; B ilk
;EXIT B EXIT
;Bdizi dcd 1,2,3,4,5,5,6,6,7,7
;Adizi dcd 2,4,6,8,10,12,14,16

    ;DIZIDEKI EN BUYUK ELEMAN
	;DIZININ BALANGIC ADRESI R6
	;DIZININ BOYUTU R7
	;BNE ESIT DEGIL , BLE KUCUK ISE 
	;BHI BUYUK ISE , BILE KUCUK ESIT 
 ;A)
 LDR R1,=CDIZI 
;0X80000000,0X800000FF
;    MOV R7,#26
;	MOV R0,#0
;ILK	
;	  LDR R5,[R1],#4
;	  CMP r0,R5
;      blt	sonra 
;	  add r8,r8,#1
;sonra  subs R7,R7,#1  
;	  BNE ILK
;	  

;B)
  
   MOV R0,#0
   MOV R2,#0
   MOV R3,#0
   MOV R4,#0
   MOV R5,#0
   MOV R7,#5
   MOV R9,#0
ILK
  LDR R8,[R1],#4
  ADD R0,R0,R8
  SUBS R7,R7,#1
  BNE ILK
    MOV R7,#5	
ILK1
  LDR R8,[R1],#4
  ADD R2,R2,R8
  SUBS R7,R7,#1
  BNE ILK1
    MOV R7,#5
ILK2
  LDR R8,[R1],#4
  ADD R3,R3,R8
  SUBS R7,R7,#1
  BNE ILK2
    MOV R7,#5
ILK3
  LDR R8,[R1],#4
  ADD R4,R4,R8
  SUBS R7,R7,#1
  BNE ILK3
    MOV R7,#5
ILK4
  LDR R8,[R1],#4
  ADD R5,R5,R8
  SUBS R7,R7,#1
  BNE ILK4
  
  LDR R7,=0X80000000 ;0X80000000,0X800000FF  o aders araligi tuttum  ondan  en buyuk  sayi alayim 
  STR R0,[R7],#4
  STR R2,[R7],#4
  STR R3,[R7],#4
  STR R4,[R7],#4
  STR R5,[R7],#4
 
	LDR R6,=0X80000000 
	MOV R7,#5
	MOV R1,#0
	LDR R3,[R6]
	
loop ADD R1,R1,#4
	 LDR R5,[R6,R1]
	 CMP R3,R5
     BHI sonra
	 MOV R11,R5
sonra subs R7,R7,#1
      CMP R7,#0
	  BNE loop

;c)
;  MOV R0,#40
;  MOV R7,#26
;  MOV R3,#0
;  BL  inputfonuction
;exit b exit

;inputfonuction 
;loop	
;	   LDR R5,[R1,R3]
;	   CMP R0,R5
;       BNE	sonra 
;	   MOV r12, r3, ASR #2 ; r12 = r3/4 her dafe 4 artirdigimiz icin bunu kullandim  4 bolum ki indexini bulayim 
;sonra  subs R7,R7,#1  
;       ADD R3,R3,#4
;	   BNE loop
;       BX LR

CDIZI DCD 10 , 1, 1, 17, 1
	  DCD 2 , -2, 2, 2, 2
	  DCD 3 , 30, -13, 3, 3
	  DCD 4 , -4, 4, 40, 4
	  DCD 5 , -50, 5, 5, 5
    END
	