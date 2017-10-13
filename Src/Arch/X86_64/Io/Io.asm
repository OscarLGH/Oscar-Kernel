global In8
global Out8
global In16
global Out16
global In32
global Out32

;UINT8 In8(UINT16 Port);
In8:
	mov dx,di
	in al,dx
	ret
;void Out8(UINT16 Port,UINT8 Value);
Out8:
	mov dx,di
	mov al,sil
	out dx,al
	ret
;UINT16 In16(UINT16 Port);	
In16:
	mov dx,di
	in ax,dx
	ret
;void Out16(UINT16 Port,UINT16 Value);
Out16:
	mov dx,di
	mov ax,si
	out dx,ax
	ret
;UINT32 In32(UINT16 Port);	
In32:
	mov dx,di
	in eax,dx
	ret
;void Out32(UINT16 Port,UINT32 Value);
Out32:
	mov dx,di
	mov eax,esi
	out dx,eax
	ret
	

