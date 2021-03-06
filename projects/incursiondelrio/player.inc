player_y			equ 64+20

player_collision:	db 0

player_column:		db 15
player_index:   	db 4

player_dir:			db 0

player_ptr:			dw player_bank_ltr_dispatch + 2; player_up_ltr_dispatch+2

playerXspeed:		db 0
playerXaccu:		db 255	

YSPEED_DEFAULT		equ 76
YSPEED_MAX			equ 128
YSPEED_MIN			equ 48


playerYspeed:		db YSPEED_DEFAULT
playerYaccu:		db 0

ScrollAccu:
	mvi d, 0
	lda playerYspeed
	mov b, a
	lda playerYaccu
	add b
	jnc saccu_1
	inr d
saccu_1:
	add b
	jnc saccu_2
	inr d
saccu_2:
	add b
	jnc saccu_3
	inr d
saccu_3:
	add b
	jnc saccu_4
	inr d
saccu_4:
	sta playerYaccu
	ret


PlayerSpeed:
	lda input_up
	ora a
	jnz ps_throttle
	lda input_down
	ora a
	jnz ps_brake
ps_spring:
	lda playerYspeed
	cpi YSPEED_DEFAULT
	mvi b, 0
	jz  ps_spring_add
	mvi b, $1; $4 ; $8
	jm 	ps_spring_add
	mvi b, $ff; $fc ; $f8
ps_spring_add:
	add b
	jmp ps_exit_sta

ps_throttle:
	lda playerYspeed
	cpi YSPEED_MAX
	jm ps_throttle_1
	jmp ps_exit
ps_throttle_1:
	adi 1; 4; 8
	sta playerYspeed
	jmp ps_exit

ps_brake:
	lda playerYspeed
	cpi YSPEED_MIN
	jnc  ps_brake_1
	;jnz ps_brake_1
	jmp ps_exit
ps_brake_1:
	sui 1;4; 8
ps_exit_sta:
	sta playerYspeed
ps_exit:
	ret

PlayerMotion:
	mvi d, $ff
	lda input_left
	ora a
	jnz pm_input_leftright
	mvi d, 1
	lda input_right
	ora a
	jnz pm_input_leftright
	
	; joystick left/right released, clear X speed
	xra a
	sta playerXspeed
	dcr a
	sta playerXaccu
	
	; set the plane straight
	lxi h, player_up_ltr_dispatch+2
	shld player_ptr
	jmp pm_Exit

pm_input_leftright:
	lda playerXspeed
	cpi $80 			; X speed maxed out?
	jz pm_x_accumulate	; yes, no change
	adi $4 				; increase speed
	sta playerXspeed
pm_x_accumulate:
	mov b, a
	mvi c, 0
	lda playerXaccu
	add b
	jnc pm_ilr_1
	dcr c
pm_ilr_1:
	add b
	sta playerXaccu
	jc  pm_playerXstep
	mov a, c
	ora a
	jm  pm_playerXstep
	jmp pm_Exit

	; perform X step, d = direction
pm_playerXstep:
	mov a, d
	ora a
	jm pm_left
	jmp pm_right

pm_left:
	lxi h, player_bank_rtl_dispatch
	shld player_ptr

	lxi h, player_index
	mov a, m
	sui 2 				; move in steps of 2
	jp pm_left_m1
	adi 8
	mov m, a
	dcx h
	dcr m  				; decrement column
	jmp pm_Exit
pm_left_m1:
	mov m, a
	jmp pm_Exit

pm_right:
	lxi h, player_bank_ltr_dispatch+2
	shld player_ptr

	lxi h, player_index
	mov a, m
	adi 2 				; move in steps of 2
	cpi 8
	jm pm_right_m1
	sui 8
	mov m, a
	dcx h
	inr m
	jmp pm_Exit
pm_right_m1:
	mov m, a
pm_Exit:
	ret

PlayerSprite:
	lxi h, $8000
	shld psw_layer+1
	lda frame_scroll
	mov b, a
	call PlayerSprite_W
	mov a, d
	sta player_collision

	lxi h, $a000
	shld psw_layer+1
	lda frame_scroll
	mov b, a
	call PlayerSprite_W
	lda player_collision
	add d
	sta player_collision

	lxi h, $c000
	shld psx_layer+1
	call PlayerSprite_X
	lda player_collision
	add d
	sta player_collision
	ret

PlayerWipe:
	lxi h, $c000
	shld psc_layer+1
	lda frame_scroll_prev
	mov b, a
	call PlayerSprite_C
	ret


PlayerSprite_W:

	; hl -> player_ptr
	lhld player_ptr
	mvi d, 0
	lda player_index
	ora a
	ral 
	mov e, a
	dad d
	; hl -> player_ptr[player_index]

	mov e, m
	inx h
	mov d, m
	; de -> &bits

	lxi h, 0
	dad sp 
	shld psw_restoreSP+1

psw_layer:
	lxi h, $c000
	lda player_column
	add h
	mov h, a

	mvi a, player_y
	add b  ; frame_scroll in b
	mov l, a
	; hl -> screen location

	xchg	
	sphl
	xchg

	mvi d, 0

	mov e, l
	;; le big pop
	pop b
	mov a, b
	cma  			; a = mask
	ana m 			; a = m & mask
					; collision = m != (m & mask)
	cmp m
	jz .+4
	inr d

	mov m, a
	dcr l
	mov a, c
	cma
	ana m
	cmp m
	jz .+4
	inr d
	mov m, a
	dcr l
	;
	pop b
	mov a, b
	cma 
	ana m
	cmp m
	jz .+4
	inr d
	mov m, a
	dcr l
	mov a, c
	cma
	ana m
	cmp m
	jz .+4
	inr d
	mov m, a
	dcr l
	;
	pop b
	mov a, b
	cma 
	ana m
	cmp m
	jz .+4
	inr d
	mov m, a
	dcr l
	mov a, c
	cma
	ana m
	cmp m
	jz .+4
	inr d
	mov m, a
	dcr l
	;
	pop b
	mov a, b
	cma 
	ana m
	cmp m
	jz .+4
	inr d
	mov m, a
	dcr l
	mov a, c
	cma
	ana m
	cmp m
	jz .+4
	inr d
	mov m, a
	dcr l
	;
	pop b
	mov a, b
	cma 
	ana m
	cmp m
	jz .+4
	inr d
	mov m, a
	dcr l
	mov a, c
	cma
	ana m
	cmp m
	jz .+4
	inr d
	mov m, a
	dcr l
	;
	pop b
	mov a, b
	cma 
	ana m
	cmp m
	jz .+4
	inr d
	mov m, a
	dcr l
	mov a, c
	cma
	ana m
	cmp m
	jz .+4
	inr d
	mov m, a

	inr h
	mov l, e
	; column end

	;; le big pop
	pop b
	mov a, b
	cma  			; a = mask
	ana m 			; a = m & mask
					; collision = m != (m & mask)
	cmp m
	jz .+4
	inr d

	mov m, a
	dcr l
	mov a, c
	cma
	ana m
	cmp m
	jz .+4
	inr d
	mov m, a
	dcr l
	;
	pop b
	mov a, b
	cma 
	ana m
	cmp m
	jz .+4
	inr d
	mov m, a
	dcr l
	mov a, c
	cma
	ana m
	cmp m
	jz .+4
	inr d
	mov m, a
	dcr l
	;
	pop b
	mov a, b
	cma 
	ana m
	cmp m
	jz .+4
	inr d
	mov m, a
	dcr l
	mov a, c
	cma
	ana m
	cmp m
	jz .+4
	inr d
	mov m, a
	dcr l
	;
	pop b
	mov a, b
	cma 
	ana m
	cmp m
	jz .+4
	inr d
	mov m, a
	dcr l
	mov a, c
	cma
	ana m
	cmp m
	jz .+4
	inr d
	mov m, a
	dcr l
	;
	pop b
	mov a, b
	cma 
	ana m
	cmp m
	jz .+4
	inr d
	mov m, a
	dcr l
	mov a, c
	cma
	ana m
	cmp m
	jz .+4
	inr d
	mov m, a
	dcr l
	;
	pop b
	mov a, b
	cma 
	ana m
	cmp m
	jz .+4
	inr d
	mov m, a
	dcr l
	mov a, c
	cma
	ana m
	cmp m
	jz .+4
	inr d
	mov m, a

	inr h
	mov l, e
	; column end
	;; le big pop
	pop b
	mov a, b
	cma  			; a = mask
	ana m 			; a = m & mask
					; collision = m != (m & mask)
	cmp m
	jz .+4
	inr d

	mov m, a
	dcr l
	mov a, c
	cma
	ana m
	cmp m
	jz .+4
	inr d
	mov m, a
	dcr l
	;
	pop b
	mov a, b
	cma 
	ana m
	cmp m
	jz .+4
	inr d
	mov m, a
	dcr l
	mov a, c
	cma
	ana m
	cmp m
	jz .+4
	inr d
	mov m, a
	dcr l
	;
	pop b
	mov a, b
	cma 
	ana m
	cmp m
	jz .+4
	inr d
	mov m, a
	dcr l
	mov a, c
	cma
	ana m
	cmp m
	jz .+4
	inr d
	mov m, a
	dcr l
	;
	pop b
	mov a, b
	cma 
	ana m
	cmp m
	jz .+4
	inr d
	mov m, a
	dcr l
	mov a, c
	cma
	ana m
	cmp m
	jz .+4
	inr d
	mov m, a
	dcr l
	;
	pop b
	mov a, b
	cma 
	ana m
	cmp m
	jz .+4
	inr d
	mov m, a
	dcr l
	mov a, c
	cma
	ana m
	cmp m
	jz .+4
	inr d
	mov m, a
	dcr l
	;
	pop b
	mov a, b
	cma 
	ana m
	cmp m
	jz .+4
	inr d
	mov m, a
	dcr l
	mov a, c
	cma
	ana m
	cmp m
	jz .+4
	inr d
	mov m, a

	inr h
	mov l, e
	; column end


psw_restoreSP:
	lxi sp, 0
	ret


PlayerSprite_X:

	; hl -> player_ptr
	lhld player_ptr
	mvi d, 0
	lda player_index
	ora a
	ral 
	mov e, a
	dad d
	; hl -> player_ptr[player_index]

	mov e, m
	inx h
	mov d, m
	; de -> &bits

	lxi h, 0
	dad sp 
	shld psx_restoreSP+1

psx_layer:
	lxi h, $c000
	lda player_column
	add h
	mov h, a

	mvi b, player_y
	lda frame_scroll
	add b
	mov l, a
	; hl -> screen location

	xchg	
	sphl
	xchg

	mov e, l
	mvi d, 0
	;; le big pop
	pop b
	mov a, b
	cma 
	ana m
	cmp m
	jz .+4
	inr d
	ora b
	mov m, a
	dcr l
	mov a, c
	cma
	ana m
	cmp m
	jz .+4
	inr d
	ora c
	mov m, a
	dcr l
	;
	pop b
	mov a, b
	cma 
	ana m
	cmp m
	jz .+4
	inr d
	ora b
	mov m, a
	dcr l
	mov a, c
	cma
	ana m
	cmp m
	jz .+4
	inr d
	ora c
	mov m, a
	dcr l
	;
	pop b
	mov a, b
	cma 
	ana m
	cmp m
	jz .+4
	inr d
	ora b
	mov m, a
	dcr l
	mov a, c
	cma
	ana m
	cmp m
	jz .+4
	inr d
	ora c
	mov m, a
	dcr l
	;
	pop b
	mov a, b
	cma 
	ana m
	cmp m
	jz .+4
	inr d
	ora b
	mov m, a
	dcr l
	mov a, c
	cma
	ana m
	cmp m
	jz .+4
	inr d
	ora c
	mov m, a
	dcr l
	;
	pop b
	mov a, b
	cma 
	ana m
	cmp m
	jz .+4
	inr d
	ora b
	mov m, a
	dcr l
	mov a, c
	cma
	ana m
	cmp m
	jz .+4
	inr d
	ora c
	mov m, a
	dcr l
	;
	pop b
	mov a, b
	cma 
	ana m
	cmp m
	jz .+4
	inr d
	ora b
	mov m, a
	dcr l
	mov a, c
	cma
	ana m
	cmp m
	jz .+4
	inr d
	ora c
	mov m, a
	dcr l

	inr h
	mov l, e
	; column end

	;; le big pop
	pop b
	mov a, b
	cma 
	ana m
	cmp m
	jz .+4
	inr d
	ora b
	mov m, a
	dcr l
	mov a, c
	cma
	ana m
	cmp m
	jz .+4
	inr d
	ora c
	mov m, a
	dcr l
	;
	pop b
	mov a, b
	cma 
	ana m
	cmp m
	jz .+4
	inr d
	ora b
	mov m, a
	dcr l
	mov a, c
	cma
	ana m
	cmp m
	jz .+4
	inr d
	ora c
	mov m, a
	dcr l
	;
	pop b
	mov a, b
	cma 
	ana m
	cmp m
	jz .+4
	inr d
	ora b
	mov m, a
	dcr l
	mov a, c
	cma
	ana m
	cmp m
	jz .+4
	inr d
	ora c
	mov m, a
	dcr l
	;
	pop b
	mov a, b
	cma 
	ana m
	cmp m
	jz .+4
	inr d
	ora b
	mov m, a
	dcr l
	mov a, c
	cma
	ana m
	cmp m
	jz .+4
	inr d
	ora c
	mov m, a
	dcr l
	;
	pop b
	mov a, b
	cma 
	ana m
	cmp m
	jz .+4
	inr d
	ora b
	mov m, a
	dcr l
	mov a, c
	cma
	ana m
	cmp m
	jz .+4
	inr d
	ora c
	mov m, a
	dcr l
	;
	pop b
	mov a, b
	cma 
	ana m
	cmp m
	jz .+4
	inr d
	ora b
	mov m, a
	dcr l
	mov a, c
	cma
	ana m
	cmp m
	jz .+4
	inr d
	ora c
	mov m, a
	dcr l

	inr h
	mov l, e
	; column end
	;; le big pop
	pop b
	mov a, b
	cma 
	ana m
	cmp m
	jz .+4
	inr d
	ora b
	mov m, a
	dcr l
	mov a, c
	cma
	ana m
	cmp m
	jz .+4
	inr d
	ora c
	mov m, a
	dcr l
	;
	pop b
	mov a, b
	cma 
	ana m
	cmp m
	jz .+4
	inr d
	ora b
	mov m, a
	dcr l
	mov a, c
	cma
	ana m
	cmp m
	jz .+4
	inr d
	ora c
	mov m, a
	dcr l
	;
	pop b
	mov a, b
	cma 
	ana m
	cmp m
	jz .+4
	inr d
	ora b
	mov m, a
	dcr l
	mov a, c
	cma
	ana m
	cmp m
	jz .+4
	inr d
	ora c
	mov m, a
	dcr l
	;
	pop b
	mov a, b
	cma 
	ana m
	cmp m
	jz .+4
	inr d
	ora b
	mov m, a
	dcr l
	mov a, c
	cma
	ana m
	cmp m
	jz .+4
	inr d
	ora c
	mov m, a
	dcr l
	;
	pop b
	mov a, b
	cma 
	ana m
	cmp m
	jz .+4
	inr d
	ora b
	mov m, a
	dcr l
	mov a, c
	cma
	ana m
	cmp m
	jz .+4
	inr d
	ora c
	mov m, a
	dcr l
	;
	pop b
	mov a, b
	cma 
	ana m
	cmp m
	jz .+4
	inr d
	ora b
	mov m, a
	dcr l
	mov a, c
	cma
	ana m
	cmp m
	jz .+4
	inr d
	ora c
	mov m, a
	dcr l

	inr h
	mov l, e
	; column end


psx_restoreSP:
	lxi sp, 0
	ret



PlayerSprite_C:

	; hl -> player_ptr
	lhld player_ptr
	mvi d, 0
	lda player_index
	ora a
	ral 
	mov e, a
	dad d
	; hl -> player_ptr[player_index]

	mov e, m
	inx h
	mov d, m
	; de -> &bits

	lxi h, 0
	dad sp 
	shld psc_restoreSP+1

psc_layer:
	lxi h, $c000
	lda player_column
	add h
	mov h, a

	mvi a, player_y
	add b  ; frame_scroll in b
	mov l, a
	; hl -> screen location

	xchg	
	sphl
	xchg

	mvi d, 0

	mov e, l
	;; le big pop
	pop b
	mov a, b
	cma  			; a = mask
	ana m 			; a = m & mask
					; collision = m != (m & mask)
	cmp m
	jz .+4
	inr d

	mov m, a
	dcr l
	mov a, c
	cma
	ana m
	mov m, a
	dcr l
	;
	pop b
	mov a, b
	cma 
	ana m
	mov m, a
	dcr l
	mov a, c
	cma
	ana m
	mov m, a
	dcr l
	;
	pop b
	mov a, b
	cma 
	ana m
	mov m, a
	dcr l
	mov a, c
	cma
	ana m
	mov m, a
	dcr l
	;
	pop b
	mov a, b
	cma 
	ana m
	mov m, a
	dcr l
	mov a, c
	cma
	ana m
	mov m, a
	dcr l
	;
	pop b
	mov a, b
	cma 
	ana m
	mov m, a
	dcr l
	mov a, c
	cma
	ana m
	mov m, a
	dcr l
	;
	pop b
	mov a, b
	cma 
	ana m
	mov m, a
	dcr l
	mov a, c
	cma
	ana m
	mov m, a

	inr h
	mov l, e
	; column end

	;; le big pop
	pop b
	mov a, b
	cma  			; a = mask
	ana m 			; a = m & mask
					; collision = m != (m & mask)
	cmp m
	jz .+4
	inr d

	mov m, a
	dcr l
	mov a, c
	cma
	ana m
	mov m, a
	dcr l
	;
	pop b
	mov a, b
	cma 
	ana m
	mov m, a
	dcr l
	mov a, c
	cma
	ana m
	mov m, a
	dcr l
	;
	pop b
	mov a, b
	cma 
	ana m
	mov m, a
	dcr l
	mov a, c
	cma
	ana m
	mov m, a
	dcr l
	;
	pop b
	mov a, b
	cma 
	ana m
	mov m, a
	dcr l
	mov a, c
	cma
	ana m
	mov m, a
	dcr l
	;
	pop b
	mov a, b
	cma 
	ana m
	mov m, a
	dcr l
	mov a, c
	cma
	ana m
	mov m, a
	dcr l
	;
	pop b
	mov a, b
	cma 
	ana m
	mov m, a
	dcr l
	mov a, c
	cma
	ana m
	mov m, a

	inr h
	mov l, e
	; column end
	;; le big pop
	pop b
	mov a, b
	cma  			; a = mask
	ana m 			; a = m & mask
					; collision = m != (m & mask)
	cmp m
	jz .+4
	inr d

	mov m, a
	dcr l
	mov a, c
	cma
	ana m
	mov m, a
	dcr l
	;
	pop b
	mov a, b
	cma 
	ana m
	mov m, a
	dcr l
	mov a, c
	cma
	ana m
	mov m, a
	dcr l
	;
	pop b
	mov a, b
	cma 
	ana m
	mov m, a
	dcr l
	mov a, c
	cma
	ana m
	mov m, a
	dcr l
	;
	pop b
	mov a, b
	cma 
	ana m
	mov m, a
	dcr l
	mov a, c
	cma
	ana m
	mov m, a
	dcr l
	;
	pop b
	mov a, b
	cma 
	ana m
	mov m, a
	dcr l
	mov a, c
	cma
	ana m
	mov m, a
	dcr l
	;
	pop b
	mov a, b
	cma 
	ana m
	mov m, a
	dcr l
	mov a, c
	cma
	ana m
	mov m, a
	; column end


psc_restoreSP:
	lxi sp, 0
	ret
