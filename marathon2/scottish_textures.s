;struct _vertical_polygon_data
;{
;	short downshift;
;	short x0;
;	short width;
;	
;	short pad;
;};
data_downshift	EQU	0
data_x0			EQU	2
data_width		EQU	4
data_pad		EQU	6

;struct bitmap_definition
;{
;	short width, height; /* in pixels */
;	short bytes_per_row; /* if ==NONE this is a transparent RLE shape */
;	
;	short flags; /* [column_order.1] [unused.15] */
;	short bit_depth; /* should always be ==8 */
;
;	short unused[8];
;	
;	pixel8 *row_addresses[1];
;};
bitmap_width			EQU	0
bitmap_height			EQU	2
bitmap_bytes_per_row	EQU	4
bitmap_flags			EQU	6
bitmap_bit_depth		EQU	8
bitmap_row_addresses	EQU 26

;struct _vertical_polygon_line_data
;{
;	void *shading_table;
;	pixel8 *texture;
;	long texture_y, texture_dy;
;};
line_shading_table		EQU	0
line_texture			EQU	4
line_texture_y			EQU	8
line_texture_dy			EQU	12

				export	._texture_vertical_polygon_lines8[DS]
				export	._texture_vertical_polygon_lines8
	
				toc 

				csect	_texture_vertical_polygon_lines8[DS]
				dc.l	._texture_vertical_polygon_lines8, TOC[tc0]

				toc 
				tc		_texture_vertical_polygon_lines8[TC], _texture_vertical_polygon_lines8[DS]

				csect [PR]

; parameters
;	r3	struct bitmap_definition *screen
;	r4	struct view_data *view
;	r5	struct _vertical_polygon_data *data
;	r6	short *y0_table
;	r7	short *y1_table

screen		EQU		r3
view		EQU		r4		; unused
data		EQU		r5
y0_table	EQU		r6
y1_table	EQU		r7

._texture_vertical_polygon_lines8:
				stmw      r13,-0x004C(SP)
				stwu      SP,-0x0110(SP)
				addi      r4,r5,8
				stw       r4,0x0038(SP)
				li        r12,0
				lha       r8,0x0004(r5)
				stw       r8,0x003C(SP)
				cmpwi     r8,0
				lha       r9,0x0002(r5)
				stb       r12,0x0044(SP)
				lha       r11,0x0004(r3)
				stw       r9,0x0040(SP)
				lha       r10,0x0000(r5)
				ble       bail
outer_loop_start:				
				lwz       r5,0x003C(SP)
				lwz       r4,0x0038(SP)
				cmpwi     r5,4
				lha       r17,0x0000(r6)
				lwz       r9,0x0008(r4)
				lwz       r28,0x000C(r4)
				lwz       r29,0x0004(r4)
				lwz       r30,0x0000(r4)
				lha       r12,0x0000(r7)
				blt       do_trivial_loop
				lwz       r4,0x0040(SP)
				clrlwi.   r4,r4,30
				bne       do_trivial_loop
				lbz       r4,0x0044(SP)
				cmpwi     r4,0
				beq       skip_trivial_loop
do_trivial_loop:
				lwz       r0,0x0040(SP)
				subc.     r12,r12,r17
				slwi      r4,r17,2
				li        r8,0
				stb       r8,0x0044(SP)
				add       r5,r3,r4
				lwz       r8,0x001A(r5)
				addi      r6,r6,2
				addi      r7,r7,2
				add       r8,r0,r8
				ble       trivial_loop_skip
				mtctr     r12
				subc      r12,r8,r11
trivial_loop:	srw       r4,r9,r10
				lbzx      r5,r29,r4
				add       r9,r9,r28
				lbzx      r8,r30,r5
				stbux     r8,r12,r11
				bdnz      trivial_loop
trivial_loop_skip:
				lwz       r8,0x003C(SP)
				lwz       r4,0x0040(SP)
				subi      r8,r8,1
				lwz       r5,0x0038(SP)
				addi      r4,r4,1
				stw       r8,0x003C(SP)
				addi      r5,r5,16
				stw       r4,0x0040(SP)
				cmpwi     cr6,r8,0
				stw       r5,0x0038(SP)
				b         outer_loop_end
skip_trivial_loop:
				lha       r0,0x0002(r6)
				stw       r0,0x0048(SP)
				cmpw      r17,r0
				lwz       r27,0x0038(SP)
				stw       r0,0x004C(SP)
				lha       r16,0x0004(r6)
				lwz       r8,0x0018(r27)
				stw       r16,0x0050(SP)
				lwz       r20,0x001C(r27)
				lwz       r26,0x0014(r27)
				lwz       r25,0x0010(r27)
				lwz       r5,0x0028(r27)
				lwz       r19,0x002C(r27)
				lwz       r24,0x0024(r27)
				lwz       r23,0x0020(r27)
				lwz       r4,0x0038(r27)
				lwz       r18,0x003C(r27)
				lwz       r22,0x0034(r27)
				lwz       r21,0x0030(r27)
				lha       r27,0x0006(r6)
				stw       r27,0x0054(SP)
				lha       r0,0x0002(r7)
				sth       r0,0x0058(SP)
				cmpw      cr6,r12,r0
				lha       r0,0x0004(r7)
				sth       r0,0x005A(SP)
				lha       r0,0x0006(r7)
				sth       r0,0x005C(SP)
				ble       sync_max1
				stw       r17,0x004C(SP)
sync_max1:		lwz       r0,0x004C(SP)
				cmpw      r0,r16
				ble       sync_max2
				lwz       r16,0x004C(SP)
sync_max2:		cmpw      r16,r27
				ble       sync_max3
				mr        r27,r16
sync_max3:		slwi      r0,r27,2
				add       r16,r3,r0
				lwz       r0,0x0040(SP)
				lwz       r16,0x001A(r16)
				add       r16,r16,r0
				ble       cr6,sync_min1
				lha       r12,0x0058(SP)
sync_min1:		lha       r0,0x005A(SP)
				cmpw      r12,r0
				ble       sync_min2
				lha       r12,0x005A(SP)
sync_min2:		lha       r0,0x005C(SP)
				cmpw      r12,r0
				ble       sync_min3
				lha       r12,0x005C(SP)
sync_min3:		cmpw      r12,r27
				bgt       sync_loops
				lwz       r4,0x003C(SP)
				li        r5,1
				stb       r5,0x0044(SP)
				cmpwi     cr6,r4,0
				b         outer_loop_end
sync_loops:		slwi      r0,r17,2
				subc.     r12,r27,r17
				add       r17,r3,r0
				lwz       r0,0x0040(SP)
				lwz       r17,0x001A(r17)
				li        r15,255
				add       r17,r0,r17
				li        r14,-1
				li        r13,-1
				subis     r0,r15,1
				stw       r0,0x0060(SP)
				subis     r0,r14,255
				stw       r0,0x0064(SP)
				addis     r0,r13,256
				stw       r0,0x0068(SP)
				ble       sync_loop_skip1
				mtctr     r12
				subc      r12,r17,r11
sync_loop1:		srw       r17,r9,r10
				lbzx      r15,r29,r17
				add       r9,r9,r28
				lbzx      r14,r30,r15
				stbux     r14,r12,r11
				bdnz      sync_loop1
sync_loop_skip1:
				lwz       r0,0x0048(SP)
				subc.     r12,r27,r0
				slwi      r0,r0,2
				add       r17,r3,r0
				lwz       r0,0x0040(SP)
				lwz       r17,0x001A(r17)
				add       r17,r17,r0
				ble       sync_loop_skip2
				mtctr     r12
				subc      r15,r17,r11
				addi      r12,r15,1
sync_loop2:		srw       r17,r8,r10
				lbzx      r15,r26,r17
				add       r8,r8,r20
				lbzx      r14,r25,r15
				stbux     r14,r12,r11
				bdnz      sync_loop2
sync_loop_skip2:
				lwz       r0,0x0050(SP)
				subc.     r12,r27,r0
				slwi      r0,r0,2
				add       r17,r3,r0
				lwz       r0,0x0040(SP)
				lwz       r17,0x001A(r17)
				add       r17,r17,r0
				ble       sync_loop_skip3
				mtctr     r12
				subc      r15,r17,r11
				addi      r12,r15,2
sync_loop3:		srw       r17,r5,r10
				lbzx      r15,r24,r17
				add       r5,r5,r19
				lbzx      r14,r23,r15
				stbux     r14,r12,r11
				bdnz      sync_loop3
sync_loop_skip3:
				lwz       r0,0x0054(SP)
				subc.     r12,r27,r0
				slwi      r0,r0,2
				add       r17,r3,r0
				lwz       r0,0x0040(SP)
				lwz       r17,0x001A(r17)
				add       r17,r17,r0
				ble       sync_loop_skip4
				mtctr     r12
				subc      r15,r17,r11
				addi      r12,r15,3
sync_loop4:		srw       r17,r4,r10
				lbzx      r15,r22,r17
				add       r4,r4,r18
				lbzx      r14,r21,r15
				stbux     r14,r12,r11
				bdnz      sync_loop4
sync_loop_skip4:
				lha       r0,0x0004(r7)
				lha       r12,0x0000(r7)
				subc      r0,r0,r27
				lha       r15,0x0002(r7)
				subc      r17,r12,r27
				stw       r0,0x0070(SP)
				subc      r0,r15,r27
				lha       r15,0x0006(r7)
				stw       r0,0x006C(SP)
				cmpw      r17,r0
				subc      r0,r15,r27
				stw       r0,0x0074(SP)
				ble       min1
				lwz       r17,0x006C(SP)
min1:			lwz       r15,0x0070(SP)
				cmpw      r17,r15
				ble       min2
				lwz       r17,0x0070(SP)
min2:			lwz       r15,0x0074(SP)
				cmpw      r17,r15
				ble       min3
				lwz       r17,0x0074(SP)
min3:			srw       r0,r4,r10
				stw       r0,0x009C(SP)
				srw       r15,r5,r10
				stw       r0,0x0078(SP)
				srw       r0,r8,r10
				stw       r15,0x00A0(SP)
				srw       r14,r9,r10
				stw       r15,0x007C(SP)
				cmpwi     r17,0
				lwz       r15,0x009C(SP)
				stw       r0,0x00A4(SP)
				stw       r0,0x0080(SP)
				lbzx      r0,r15,r22
				stw       r14,0x00A8(SP)
				stw       r14,0x0084(SP)
				lbzx      r0,r21,r0
				lwz       r31,0x0060(SP)
				and       r0,r0,r31
				lwz       r31,0x00A0(SP)
				stw       r0,0x00AC(SP)
				lbzx      r0,r31,r24
				lbzx      r0,r23,r0
				lwz       r31,0x00AC(SP)
				slwi      r0,r0,8
				or        r0,r31,r0
				lwz       r31,0x0064(SP)
				and       r0,r0,r31
				lwz       r31,0x00A4(SP)
				stw       r0,0x00B0(SP)
				lbzx      r0,r31,r26
				lbzx      r0,r25,r0
				lwz       r31,0x00B0(SP)
				slwi      r0,r0,16
				or        r0,r31,r0
				lwz       r31,0x0068(SP)
				and       r0,r0,r31
				lwz       r31,0x00A8(SP)
				stw       r0,0x00B4(SP)
				lbzx      r0,r31,r29
				lwz       r15,0x009C(SP)
				lbzx      r0,r30,r0
				lwz       r31,0x00B4(SP)
				slwi      r0,r0,24
				or        r31,r31,r0
				lwz       r0,0x00A4(SP)
				add       r0,r17,r27
				lwz       r27,0x00A0(SP)
				stw       r0,0x0088(SP)
				ble       desync
				stw       r3,0x008C(SP)
				mtctr     r17
				stw       r6,0x0094(SP)
				subc      r12,r16,r11
				lwz       r3,0x0090(SP)
				stw       r7,0x0098(SP)
				lwz       r6,0x0078(SP)
				lwz       r7,0x007C(SP)
inner_loop:
				lwz       r15,0x0080(SP)
				srw       r3,r4,r10
				srw       r17,r8,r10
				srw       r16,r5,r10
				cmplw     r3,r6
				cmplw     cr6,r16,r7
				cmplw     cr7,r17,r15
				srw       r27,r9,r10
				beq       second_pixel
				mr        r6,r3
				lbzx      r0,r22,r6
			;	clrrwi    r31,r31,8
				lbzx      r0,r21,r0
			;	or        r31,r31,r0
				rlwimi	  r31,r0,0,24,31	# bottom 8
second_pixel:	beq       cr6,third_pixel
				lbzx      r0,r24,r16
				mr        r7,r16
				lbzx      r0,r23,r0
			;	lwz       r16,0x0060(SP)
			;	slwi      r0,r0,8
			;	and       r31,r31,r16
			;	or        r31,r31,r0
				rlwimi	  r31,r0,8,16,23	# third 8
third_pixel		beq       cr7,fourth_pixel
				lbzx      r0,r26,r17
			;	lwz       r16,0x0064(SP)
				lbzx      r0,r25,r0
			;	and       r31,r31,r16
				stw       r17,0x0080(SP)
			;	slwi      r0,r0,16
			;	or        r31,r31,r0
				rlwimi	  r31,r0,16,8,15	# second 8
fourth_pixel:
				lwz       r17,0x0084(SP)
				cmplw     r27,r17
				beq       data_write
				lbzx      r17,r29,r27
			;	lwz       r15,0x0068(SP)
				lbzx      r16,r30,r17
			;	and       r14,r31,r15
				stw       r27,0x0084(SP)
			;	slwi      r16,r16,24
			;	or        r31,r14,r16
				rlwimi	  r31,r16,24,0,7	# top 8
data_write:		stwux     r31,r12,r11
				add       r4,r4,r18
				add       r5,r5,r19
				add       r8,r8,r20
				add       r9,r9,r28
				bdnz      inner_loop
				
				stw       r7,0x007C(SP)
				add       r16,r12,r11
				stw       r6,0x0078(SP)
				lwz       r7,0x0098(SP)
				stw       r3,0x0090(SP)
				lwz       r6,0x0094(SP)
				lwz       r3,0x008C(SP)
				lha       r12,0x0000(r7)
desync:
				lwz       r27,0x0088(SP)
				subc.     r12,r12,r27
				ble       desync_loop_skip1
				mtctr     r12
				subc      r12,r16,r11
desync_loop1:	srw       r27,r9,r10
				lbzx      r17,r29,r27
				add       r9,r9,r28
				lbzx      r15,r30,r17
				stbux     r15,r12,r11
				bdnz      desync_loop1
desync_loop_skip1:
				lha       r9,0x0002(r7)
				lwz       r30,0x0088(SP)
				subc.     r12,r9,r30
				ble       desync_loop_skip2
				mtctr     r12
				subc      r9,r16,r11
				addi      r12,r9,1
desync_loop2:	srw       r9,r8,r10
				lbzx      r30,r26,r9
				add       r8,r8,r20
				lbzx      r29,r25,r30
				stbux     r29,r12,r11
				bdnz      desync_loop2
desync_loop_skip2:
				lha       r8,0x0004(r7)
				lwz       r9,0x0088(SP)
				subc.     r12,r8,r9
				ble       desync_loop_skip3
				mtctr     r12
				subc      r8,r16,r11
				addi      r12,r8,2
desync_loop3:	srw       r8,r5,r10
				lbzx      r9,r24,r8
				add       r5,r5,r19
				lbzx      r30,r23,r9
				stbux     r30,r12,r11
				bdnz      desync_loop3
desync_loop_skip3:
				lha       r5,0x0006(r7)
				lwz       r8,0x0088(SP)
				subc.     r12,r5,r8
				ble       desync_loop_skip4
				mtctr     r12
				subc      r5,r16,r11
				addi      r12,r5,3
desync_loop4:	srw       r5,r4,r10
				lbzx      r8,r22,r5
				add       r4,r4,r18
				lbzx      r9,r21,r8
				stbux     r9,r12,r11
				bdnz      desync_loop4
desync_loop_skip4:
				lwz       r4,0x003C(SP)
				addi      r6,r6,8
				lwz       r5,0x0038(SP)
				subi      r4,r4,4
				lwz       r8,0x0040(SP)
				cmpwi     cr6,r4,0
				stw       r4,0x003C(SP)
				addi      r5,r5,64
				stw       r5,0x0038(SP)
				addi      r8,r8,4
				stw       r8,0x0040(SP)
				addi      r7,r7,8
outer_loop_end:
				bgt       cr6,outer_loop_start
bail:
				addi      SP,SP,272
				lmw       r13,-0x004C(SP)
				blr

funny_traceback:
				dc.l      0x00000000                ; --- Traceback Table ---
				dc.b      0x00                      ; traceback format version
				dc.b      0x00                      ; language: TB_C (C)
				dc.b      0x20                      ; has_tboff
				dc.b      0x40                      ; name_present | WALK_ONCOND
				dc.b      0x00                      ; fpr_saved = 0
				dc.b      0x00                      ; gpr_saved = 0
				dc.b      0x00                      ; fixedparms = 0
				dc.b      0x01                      ; floatparms = 0, parmsonstk
				dc.l      funny_traceback-._texture_vertical_polygon_lines8
				dc.w      28                        ; name length
				dc.b      'HeyPervertStopLookingAtMyPEF'
				ALIGN	2

#
# HORIZONTAL POLYGON LINES
#
#
				export	._texture_horizontal_polygon_lines8[DS]
				export	._texture_horizontal_polygon_lines8
	
				toc 

				csect	_texture_horizontal_polygon_lines8[DS]
				dc.l	._texture_horizontal_polygon_lines8, TOC[tc0]

				toc 
				tc		_texture_horizontal_polygon_lines8[TC], _texture_horizontal_polygon_lines8[DS]

				csect [PR]
._texture_horizontal_polygon_lines8:
				stmw      r22,-0x0028(SP)
				extsh     r5,r10
				extsh     r7,r7
				subi      r5,r5,1
				extsh.    r10,r5
				blt       @bail
@outer_loop_top:
				lha       r0,0x0000(r8)
				slwi      r12,r7,2
				lha       r11,0x0000(r9)
				add       r12,r4,r12
				lwz       r29,0x0010(r6)
				subc      r27,r11,r0
				lwz       r12,0x001A(r12)
				addi      r9,r9,2
				lwz       r28,0x001A(r3)
				add       r12,r0,r12
				lwz       r11,0x0000(r6)
				addi      r8,r8,2
				lwz       r5,0x0004(r6)
				clrlwi.   r0,r12,30
				lwz       r31,0x0008(r6)
				extsh     r27,r27
				lwz       r30,0x000C(r6)
				beq       @align_loop_skip
@align_loop:	rlwinm    r0,r5,14,18,24
				srwi      r26,r11,25
				subi      r27,r27,1
				add       r26,r0,r26
				lbzx      r0,r26,r28
				add       r11,r11,r31
				lbzx      r0,r29,r0
				add       r5,r5,r30
				stb       r0,0x0000(r12)
				addi      r12,r12,1
				extsh     r27,r27
				clrlwi.   r0,r12,30
				bne       @align_loop
@align_loop_skip:
				cmpwi     r27,4
				blt       @inner_loop_skip
				srawi     r0,r27,2
				extsh     r26,r0
				subi      r0,r26,1
				extsh.    r26,r0
				blt       @inner_loop_skip
				addi      r0,r26,1
				mtctr     r0
				li        r26,-256
				subi      r12,r12,4
				addis     r26,r26,1
@inner_loop:	rlwinm    r0,r5,14,18,24
				srwi      r25,r11,25
				add       r5,r5,r30
				add       r25,r0,r25
				lbzx      r0,r25,r28
				add       r11,r11,r31
				lbzx      r0,r29,r0
				rlwinm    r25,r5,14,18,24
				srwi      r24,r11,25
				add       r5,r30,r5
				add       r25,r25,r24
				lbzx      r25,r28,r25
				slwi      r0,r0,24
				add       r11,r31,r11
				lbzx      r25,r29,r25
				rlwinm    r24,r5,14,18,24
				srwi      r23,r11,25
				add       r22,r31,r11
				add       r11,r24,r23
				lbzx      r11,r28,r11
				rlwimi	  r0,r25,16,8,15	# stuff second byte
				add       r5,r30,r5
				lbzx      r24,r29,r11
				add       r11,r31,r22
				rlwinm    r23,r5,14,18,24
				srwi      r22,r22,25
				add       r23,r23,r22
				lbzx      r23,r28,r23
				rlwimi	  r0,r24,8,16,23	# stuff third byte
				lbzx      r23,r29,r23
				add       r5,r30,r5
				rlwimi	  r0,r23,0,24,31	# stuff last byte
				stwu      r0,0x0004(r12)
				bdnz      @inner_loop
				addi      r12,r12,4
@inner_loop_skip:
				clrlwi.   r26,r27,30
				beq       @tail_loop_skip
				subi      r12,r12,1
@tail_loop:		rlwinm    r0,r5,14,18,24
				srwi      r26,r11,25
				subi      r27,r27,1
				add       r26,r0,r26
				lbzx      r0,r26,r28
				extsh     r27,r27
				lbzx      r0,r29,r0
				add       r11,r11,r31
				stbu      r0,0x0001(r12)
				clrlwi.   r0,r27,30
				add       r5,r5,r30
				bne       @tail_loop
@tail_loop_skip:
				subi      r11,r10,1
				addi      r5,r7,1
				extsh.    r10,r11
				addi      r6,r6,20
				extsh     r7,r5
				bge       @outer_loop_top
@bail:			lmw       r22,-0x0028(SP)
				blr

#
# LANDSCAPE POLYGON LINES
#
#
# parameters
#	r3	struct bitmap_definition *texture
#	r4	struct bitmap_definition *screen
#	r5	struct view_data *view	unused
#	r6	struct _horizontal_polygon_line_data *data
#	r7	short y0
#	r8	short *x0_table
#	r9	short *x1_table
#	r10	short line_count

#struct _horizontal_polygon_line_data
#{
#	unsigned long source_x, source_y;
#	unsigned long source_dx, source_dy;
#	
#	void *shading_table;
#};
horiz_source_x		EQU	0
horiz_source_y		EQU	4
horiz_source_dx		EQU	8
horiz_source_dy		EQU	12
horiz_shading_table	EQU	16

				export	._landscape_horizontal_polygon_lines8[DS]
				export	._landscape_horizontal_polygon_lines8
	
				toc 

				csect	_landscape_horizontal_polygon_lines8[DS]
				dc.l	._landscape_horizontal_polygon_lines8, TOC[tc0]

				toc 
				tc		_landscape_horizontal_polygon_lines8[TC], _landscape_horizontal_polygon_lines8[DS]

				csect [PR]
._landscape_horizontal_polygon_lines8
				stmw      r25,-0x001C(SP)
				lha       r5,bitmap_height(r3)
				extsh     r7,r7
				cmpwi     r5,1024
				extsh     r10,r10
				bne       @1
				li        r29,22
				b         @2
@1:				li        r29,23
@2:				subi      r5,r10,1
				extsh.    r10,r5
				blt       @land_bail
@land_line_loop:
				lha       r0,0x0000(r8)
				slwi      r12,r7,2
				lwz       r11,horiz_source_y(r6)
				add       r12,r4,r12
				lha       r5,0x0000(r9)
				slwi      r11,r11,2
				lwz       r12,0x001A(r12)
				add       r11,r3,r11
				lwz       r31,horiz_shading_table(r6)
				add       r12,r0,r12
				lwz       r30,0x001A(r11)
				subc      r27,r5,r0
				lwz       r11,horiz_source_x(r6)
				clrlwi.   r0,r12,30
				lwz       r5,horiz_source_dx(r6)
				addi      r9,r9,2
				addi      r8,r8,2
				mr        r28,r27
				beq       @land_align_loop_skip
				subi      r28,r27,1
				cmpwi     r28,0
				blt       @land_align_loop_skip
				mtctr     r27
@land_align_loop:
				srw       r0,r11,r29
				lbzx      r0,r30,r0
				add       r11,r11,r5
				lbzx      r0,r31,r0
				stb       r0,0x0000(r12)
				addi      r12,r12,1
				clrlwi.   r0,r12,30
				beq       @57F0
				bdnz      @land_align_loop
				mfctr     r0
				subc      r0,r27,r0
				subc      r28,r28,r0
@land_align_loop_skip:
				cmpwi     r28,4
				blt       @land_inner_loop_skip
				srawi     r0,r28,2
				extsh     r27,r0
				subi      r0,r27,1
				extsh.    r27,r0
				blt       @land_inner_loop_skip
				addi      r0,r27,1
				mtctr     r0
				subi      r12,r12,4
@land_inner_loop:
				add       r0,r11,r5
				srw       r11,r11,r29
				lbzx      r11,r30,r11
				add       r27,r5,r0
				lbzx      r11,r31,r11
				srw       r0,r0,r29
				lbzx      r0,r30,r0
				srw       r26,r27,r29
				lbzx      r0,r31,r0
				add       r27,r5,r27
				lbzx      r26,r30,r26
				rlwimi	  r0,r0,16,8,15		# second byte
				lbzx      r26,r31,r26
				rlwimi	  r0,r11,24,0,7		# first byte
				srw       r25,r27,r29
				lbzx      r25,r30,r25
				rlwimi	  r0,r26,8,16,23	# third byte
				lbzx      r25,r31,r25
				add       r11,r5,r27
				rlwimi	  r0,r25,0,24,31	# fourth byte
				stwu      r0,0x0004(r12)
				bdnz      @land_inner_loop
				addi      r12,r12,4
@land_inner_loop_skip:
				mr        r27,r28
				clrlwi.   r26,r27,30
				beq       @land_tail_loop_skip
				subi      r28,r28,1
				cmpwi     r28,0
				blt       @land_tail_loop_skip
				mtctr     r27
				subi      r12,r12,1
@land_tail_loop:
				srw       r0,r11,r29
				lbzx      r0,r30,r0
				clrlwi.   r27,r28,30
				lbzx      r0,r31,r0
				add       r11,r11,r5
				stbu      r0,0x0001(r12)
				beq       @land_tail_loop_skip
				subi      r28,r28,1
				bdnz      @land_tail_loop
@land_tail_loop_skip:
				subi      r11,r10,1
				addi      r5,r7,1
				extsh.    r10,r11
				addi      r6,r6,20
				extsh     r7,r5
				bge       @land_line_loop
				lmw       r25,-0x001C(SP)
				blr
@57F0:			mfctr     r0
				subc      r0,r27,r0
				subc      r28,r28,r0
				b         @land_align_loop_skip
@land_bail:		lmw       r25,-0x001C(SP)
				blr
