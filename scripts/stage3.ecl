.header 30
.offset 0 0x007C
.offset 1 0x00AB
.offset 2 0x00DA
.offset 3 0x0109
.offset 4 0x0143
.offset 5 0x01C1
.offset 6 0x01F4
.offset 7 0x0232
.offset 8 0x0253
.offset 9 0x0274
.offset 10 0x02AA
.offset 11 0x037D
.offset 12 0x040F
.offset 13 0x07D6
.offset 14 0x0B01
.offset 15 0x007C
.offset 16 0x007C
.offset 17 0x007C
.offset 18 0x007C
.offset 19 0x007C
.offset 20 0x007C
.offset 21 0x007C
.offset 22 0x007C
.offset 23 0x007C
.offset 24 0x007C
.offset 25 0x007C
.offset 26 0x007C
.offset 27 0x007C
.offset 28 0x007C
.offset 29 0x007C

.org 0x007C
@script_0:
@script_15:  ; shared
@script_16:  ; shared
@script_17:  ; shared
@script_18:  ; shared
@script_19:  ; shared
@script_20:  ; shared
@script_21:  ; shared
@script_22:  ; shared
@script_23:  ; shared
@script_24:  ; shared
@script_25:  ; shared
@script_26:  ; shared
@script_27:  ; shared
@script_28:  ; shared
@script_29:  ; shared
    SETUP hp=270 score=5000
    ANM pattern=0 speed=0
    DEGA angle=64
    CALL jmp=@script_3
    CALL jmp=@label_0130
    NOP count=50
    LLOPEN id=0
    NOP count=70
    LLCLOSE id=0
    CALL jmp=@label_0123
.org 0x00A3
@label_00A3:
    NOP count=100
    JMP jmp=@label_00A3
.org 0x00AB
@script_1:
    SETUP hp=270 score=5000
    ANM pattern=0 speed=0
    DEGA angle=32
    CALL jmp=@script_3
    CALL jmp=@label_0130
    NOP count=50
    LLOPEN id=0
    NOP count=70
    LLCLOSE id=0
    CALL jmp=@label_0123
.org 0x00D2
@label_00D2:
    NOP count=100
    JMP jmp=@label_00D2
.org 0x00DA
@script_2:
    SETUP hp=270 score=5000
    ANM pattern=0 speed=0
    DEGA angle=96
    CALL jmp=@script_3
    CALL jmp=@label_0130
    NOP count=50
    LLOPEN id=0
    NOP count=70
    LLCLOSE id=0
    CALL jmp=@label_0123
.org 0x0101
@label_0101:
    NOP count=100
    JMP jmp=@label_0101
.org 0x0109
@script_3:
    CLIP_ON
    SPDA speed=512
    MOV count=20
.org 0x0112
@label_0112:
    MOV count=1
    SPDR speed=-16
    LOOP jmp=@label_0112 count=32
    CLIP_OFF
    RET
.org 0x0123
@label_0123:
    MOV count=1
    SPDR speed=-16
    JMP jmp=@label_0123
.org 0x0130
@label_0130:
    LDEGE
    LNUMA n=1
    LSPDA v=64
    LCOL color=0
    LWA w=1280
    LTYPE type=0
    LLSET
    RET
.org 0x0143
@script_4:
    SETUP hp=120 score=3000
    RLCHG_ON
    ANM pattern=3 speed=0
    SPDA speed=128
    DEGA angle=64
    TCMD cmd=1
    TDEGA angle=0 dw=0
    TNUMA n=6 ns=0
    TSPDA v=8 a=0
    TTYPE type=0
    TCOL color=19
    MOV count=80
    RND reg=0
    MOD reg=0 div=100
    CMPC reg=0 val=50
    JL jmp=@label_018F
.org 0x017C
@label_017C:
    TAMA
    TDEGR angle=6 dw=0
    NOP count=2
    LOOP jmp=@label_017C count=7
    JMP jmp=@label_01A2
.org 0x018F
@label_018F:
    TAMA
    TDEGR angle=-6 dw=0
    NOP count=2
    LOOP jmp=@label_018F count=7
    JMP jmp=@label_01A2
.org 0x01A2
@label_01A2:
    TCMD cmd=8
    TDEGA angle=0 dw=18
    TNUMA n=2 ns=0
    TSPDA v=10 a=0
    TCOL color=0
    TAUTO interval=16
    ROL deg=4 count=64
    TAUTO interval=0
    DEGA angle=192
.org 0x01B9
@label_01B9:
    MOV count=100
    JMP jmp=@label_01B9
.org 0x01C1
@script_5:
    SETUP hp=30 score=700
    RLCHG_ON
    CLIP_ON
    ANM pattern=2 speed=0
    SPDA speed=384
    DEGA angle=112
.org 0x01D6
@label_01D6:
    MOV count=2
    SPDR speed=-8
    LOOP jmp=@label_01D6 count=30
    CLIP_OFF
    DEGS
.org 0x01E7
@label_01E7:
    MOV count=2
    SPDR speed=8
    JMP jmp=@label_01E7
.org 0x01F4
@script_6:
    SETUP hp=30 score=700
    RLCHG_ON
    CLIP_ON
    ANM pattern=2 speed=0
    SPDA speed=384
    DEGA angle=112
.org 0x0209
@label_0209:
    MOV count=2
    SPDR speed=-8
    LOOP jmp=@label_0209 count=30
    CLIP_OFF
.org 0x0219
@label_0219:
    DEGR angle=4
    NOP count=1
    LOOP jmp=@label_0219 count=12
.org 0x0225
@label_0225:
    MOV count=2
    SPDR speed=8
    JMP jmp=@label_0225
.org 0x0232
@script_7:
    ANM pattern=4 speed=6
    HITSB_OFF
    DAMAGE_OFF
    CLIP_ON
    SPDA speed=256
    DEGA angle=0
    MOV count=12
    DEGR angle=-128
    CLIP_OFF
    NOP count=60
    NOP count=60
.org 0x024B
@label_024B:
    MOV count=100
    JMP jmp=@label_024B
.org 0x0253
@script_8:
    ANM pattern=5 speed=6
    HITSB_OFF
    DAMAGE_OFF
    CLIP_ON
    SPDA speed=256
    DEGA angle=128
    MOV count=12
    DEGR angle=-128
    CLIP_OFF
    NOP count=60
    NOP count=60
.org 0x026C
@label_026C:
    MOV count=100
    JMP jmp=@label_026C
.org 0x0274
@script_9:
    SETUP hp=15 score=500
    RLCHG_ON
    CLIP_ON
    ANM pattern=1 speed=0
    SPDA speed=320
    DEGA angle=88
    TCMD cmd=90
    TDEGA angle=0 dw=6
    TNUMA n=8 ns=0
    TSPDA v=136 a=0
    TTYPE type=0
    TCOL color=0
    TAUTO interval=255
    MOV count=30
    CLIP_OFF
    ROL deg=4 count=15
.org 0x02A2
@label_02A2:
    MOV count=100
    JMP jmp=@label_02A2
.org 0x02AA
@script_10:
    SETUP hp=1200 score=1500000
    CLIP_ON
    ANM pattern=8 speed=0
    ITEM type=3
    SPDA speed=192
    DEGA angle=64
    RND reg=0
    MOD reg=0 div=100
    CMPC reg=0 val=50
    JL jmp=@label_02DD
    XYA x=469 y=-80
    JMP jmp=@label_02E2
.org 0x02DD
@label_02DD:
    XYA x=169 y=-80
.org 0x02E2
@label_02E2:
    RLCHG_ON
    INT id=0
    MOV count=60
.org 0x02E8
@label_02E8:
    CALL jmp=@label_0342
    ROL deg=1 count=256
    ROL deg=-1 count=32
    MOV count=30
    ROL deg=1 count=128
    PSE id=1
    CALL jmp=@label_036B
    MOV count=60
    ROL deg=1 count=128
    LLOPEN id=0
    ROL deg=1 count=64
    LLCLOSE id=0
    ROL deg=1 count=64
    ROL deg=-1 count=32
    MOV count=30
    ROL deg=1 count=128
    MOV count=60
    CALL jmp=@label_0357
    ROL deg=1 count=256
    TAUTO interval=0
    ROL deg=-1 count=32
    MOV count=30
    ROL deg=1 count=128
    MOV count=60
    JMP jmp=@label_02E8
.org 0x0342
@label_0342:
    TCMD cmd=85
    TDEGA angle=0 dw=10
    TNUMA n=22 ns=4
    TSPDA v=6 a=0
    TTYPE type=4
    TVDEG vd=1
    TREP rep=80
    TCOL color=32
    TAMA
    RET
.org 0x0357
@label_0357:
    TCMD cmd=9
    TDEGA angle=0 dw=0
    TNUMA n=22 ns=1
    TSPDA v=8 a=-1
    TTYPE type=1
    TREP rep=60
    TCOL color=34
    TAUTO interval=48
    RET
.org 0x036B
@label_036B:
    LTYPE type=2
    LNUMA n=1
    LSPDA v=64
    LCOL color=0
    LWA w=1280
    LLSET
    RET
.org 0x037D
@script_11:
    SETUP hp=90000 score=1000
    ANM pattern=7 speed=0
    CLIP_ON
    TCMD cmd=92
    TDEGA angle=0 dw=12
    TNUMA n=1 ns=12
    TSPDA v=5 a=2
    TREP rep=128
    TTYPE type=1
    TCOL color=20
    JDIF easy=@label_03AC norm=@label_03B4 hard=@label_03BC luna=@label_03C4
.org 0x03AC
@label_03AC:
    TNUMA n=1 ns=2
    JMP jmp=@label_03CC
.org 0x03B4
@label_03B4:
    TNUMA n=1 ns=4
    JMP jmp=@label_03CC
.org 0x03BC
@label_03BC:
    TNUMA n=1 ns=6
    JMP jmp=@label_03CC
.org 0x03C4
@label_03C4:
    TNUMA n=1 ns=8
    JMP jmp=@label_03CC
.org 0x03CC
@label_03CC:
    MOVR dst=0 src=144
    CMPC reg=0 val=17856
    JL jmp=@label_03ED
    RND reg=0
    MOD reg=0 div=100
    CMPC reg=0 val=5
    JS jmp=@label_03F5
.org 0x03ED
@label_03ED:
    NOP count=100
    JMP jmp=@label_03CC
.org 0x03F5
@label_03F5:
    TAMA2
    NOP count=200
    JMP jmp=@label_03CC
    TCMD cmd=10
    TDEGA angle=0 dw=12
    TNUMA n=6 ns=0
    TSPDA v=0 a=2
    TREP rep=128
    TTYPE type=1
    TCOL color=32
.org 0x040F
@script_12:
    SETUP hp=28980 score=9000000
    ANM pattern=6 speed=0
    ANMEX pattern=9
    HITXY w=64 h=16
    CLIP_ON
    STI jmp=@label_04A1 vector=HP val=19980
    STI jmp=@label_04A1 vector=TIMER val=5000
    NOP count=40
.org 0x043A
@label_043A:
    MXYA x=319 y=80 count=60
    CALL jmp=@label_0817
    NOP count=10
    CALL jmp=@label_08A6
    ROL deg=2 count=128
    NOP count=30
    CALL jmp=@label_088D
    ROL deg=-2 count=128
    TAUTO interval=0
    MXYA x=238 y=40 count=80
    NOP count=60
    CALL jmp=@script_13
    MXYA x=319 y=80 count=60
    CALL jmp=@label_08A6
    ROL deg=-2 count=128
    NOP count=30
    CALL jmp=@label_088D
    ROL deg=2 count=128
    TAUTO interval=0
    MXYA x=401 y=40 count=80
    NOP count=60
    CALL jmp=@script_13
    JMP jmp=@label_043A
.org 0x04A1
@label_04A1:
    SETUP hp=19980 score=9000000
    STI jmp=@label_0619 vector=HP val=10980
    STI jmp=@label_0619 vector=TIMER val=6000
    TCLR
.org 0x04BF
@label_04BF:
    MXYA x=319 y=70 count=80
    CALL jmp=@label_0A9C
    NOP count=60
    LLOPEN id=255
    DEGA angle=64
    SPDA speed=64
    RND reg=0
    MOD reg=0 div=10
    MOVC dst=1 val=30
    ADD dst=0 src=1
    MOVC dst=1 val=1
.org 0x04EE
@label_04EE:
    MOV count=2
    CALL jmp=@label_0AF3
    SUB dst=0 src=1
    CMPC reg=0 val=0
    JL jmp=@label_04EE
    NOP count=10
    CALL jmp=@label_0A75
    NOP count=10
    RND reg=0
    MOD reg=0 div=10
    MOVC dst=1 val=10
    ADD dst=0 src=1
    MOVC dst=1 val=1
.org 0x0526
@label_0526:
    NOP count=2
    CALL jmp=@label_0AFA
    SUB dst=0 src=1
    CMPC reg=0 val=0
    JL jmp=@label_0526
    NOP count=10
    CALL jmp=@label_0A75
    NOP count=10
    RND reg=0
    MOD reg=0 div=10
    MOVC dst=1 val=10
    ADD dst=0 src=1
    MOVC dst=1 val=1
.org 0x055E
@label_055E:
    NOP count=2
    CALL jmp=@label_0AF3
    SUB dst=0 src=1
    CMPC reg=0 val=0
    JL jmp=@label_055E
    NOP count=10
    CALL jmp=@label_0A75
    NOP count=10
    NOP count=120
    LLCLOSE id=255
    MXYA x=228 y=100 count=60
    CALL jmp=@label_0ABF
    CALL jmp=@label_08BF
    NOP count=60
    LLOPEN id=255
.org 0x059A
@label_059A:
    PSE id=4
    TAMA
    NOP count=6
    TDEGR angle=-6 dw=0
    LLDEGR id=0 deg=1
    LOOP jmp=@label_059A count=20
    LLCLOSE id=255
    MXYA x=319 y=289 count=80
    CALL jmp=@label_08D4
    DEGA angle=192
    SPDA speed=32
    PSE id=14
.org 0x05C4
@label_05C4:
    MOV count=10
    TCOL color=16
    TAMA
    TCOL color=0
    TAMA
    LOOP jmp=@label_05C4 count=12
    PSE id=14
.org 0x05D6
@label_05D6:
    MOV count=10
    TCOL color=16
    TAMA
    TCOL color=0
    TAMA
    LOOP jmp=@label_05D6 count=12
    NOP count=60
    MXYA x=411 y=100 count=60
    CALL jmp=@label_0AD9
    CALL jmp=@label_08BF
    NOP count=60
    LLOPEN id=255
.org 0x05FF
@label_05FF:
    PSE id=4
    TAMA
    NOP count=6
    TDEGR angle=6 dw=0
    LLDEGR id=0 deg=-1
    LOOP jmp=@label_05FF count=20
    LLCLOSE id=255
    JMP jmp=@label_04BF
.org 0x0619
@label_0619:
    SETUP hp=10980 score=9000000
    STI jmp=@label_072C vector=HP val=1980
    STI jmp=@label_072C vector=TIMER val=5000
    TCLR
    LLCLOSE id=255
.org 0x0639
@label_0639:
    MXYA x=319 y=80 count=60
    CALL jmp=@label_08E9
    RND reg=0
    MOD reg=0 div=2000
    CMPC reg=0 val=1000
    JL jmp=@label_0663
    MOVC dst=4 val=10
    JMP jmp=@label_0669
.org 0x0663
@label_0663:
    MOVC dst=4 val=4294967286
.org 0x0669
@label_0669:
    MOVC dst=0 val=0
    MOVR dst=1 src=134
    MOVC dst=2 val=0
    MOVC dst=5 val=0
.org 0x067E
@label_067E:
    PSE id=4
    TXYR dx=-80 dy=-40
    MOVR dst=3 src=1
    ADD dst=3 src=2
    MOVR dst=134 src=3
    TAMA2
    TXYR dx=80 dy=-40
    MOVR dst=3 src=1
    ADD dst=3 src=2
    MOVR dst=134 src=3
    TAMA2
    INC reg=0
    MOD reg=0 div=4
    CMPC reg=0 val=0
    JL jmp=@label_06CD
    ADD dst=2 src=4
    NOP count=3
    INC reg=5
    MOD reg=5 div=2
    CMPC reg=5 val=0
    JL jmp=@label_06CD
    TSPDR v=1 a=0
.org 0x06CD
@label_06CD:
    NOP count=2
    LOOP jmp=@label_067E count=79
    PSE id=1
    NOP count=150
    CALL jmp=@label_0A19
    TVDEG vd=8
    TAUTO interval=32
    DEGA angle=0
    PSE id=14
    WAVX vx=64 amp=70 vd=2 count=128
    DEGA angle=0
    PSE id=14
    WAVX vx=-64 amp=70 vd=2 count=128
    DEGA angle=0
    PSE id=14
    TVDEG vd=-8
    WAVX vx=-64 amp=70 vd=2 count=128
    DEGA angle=0
    PSE id=14
    WAVX vx=64 amp=70 vd=2 count=128
    DEGA angle=0
    TAUTO interval=0
    NOP count=60
    CALL jmp=@label_091D
    JMP jmp=@label_0639
.org 0x072C
@label_072C:
    CLI vector=HP
    CLI vector=TIMER
    SETUP hp=1980 score=9000000
    TCLR
    LLCLOSE id=255
    STG3EFC
    DAMAGE_OFF
    HITSB_OFF
    NOP count=32
    DAMAGE_ON
    HITSB_ON
    MXYA x=319 y=80 count=60
    MOVC dst=0 val=128
    MOVC dst=1 val=0
    MOVC dst=2 val=22
    MOVC dst=3 val=34
    MOVC dst=6 val=0
.org 0x0769
@label_0769:
    CALL jmp=@label_0A31
.org 0x076E
@label_076E:
    PSE id=16
    TXYR dx=-80 dy=-40
    MOVR dst=134 src=0
    TAMA
    TXYR dx=80 dy=-40
    MOVR dst=134 src=1
    TAMA
    TNUMR n=1 ns=0
    NOP count=2
    LOOP jmp=@label_076E count=2
    PSE id=16
    NOP count=2
    PSE id=16
    NOP count=2
    CALL jmp=@label_0A46
    ADD dst=0 src=2
    SUB dst=1 src=2
    DEC reg=3
    CMPC reg=3 val=2
    JL jmp=@label_0769
    MOVC dst=3 val=2
    INC reg=6
    CMPC reg=6 val=28
    JS jmp=@label_0769
    SETUP hp=0 score=0  ; death marker
    TCLR
.org 0x07CE
@label_07CE:
    NOP count=100
    JMP jmp=@label_07CE
.org 0x07D6
@script_13:
    TCMD cmd=80
    TDEGA angle=32 dw=15
    TNUMA n=5 ns=0
    TSPDA v=29 a=0
    TTYPE type=0
    TCOL color=3
    TDEGS
    MOVR dst=0 src=134
    MOVR dst=1 src=134
    MOVC dst=2 val=20
    SUB dst=0 src=2
    ADD dst=1 src=2
.org 0x07F8
@label_07F8:
    PSE id=0
    TXYR dx=-80 dy=-40
    MOVR dst=134 src=0
    TAMA
    TXYR dx=80 dy=-40
    MOVR dst=134 src=1
    TAMA
    NOP count=2
    LOOP jmp=@label_07F8 count=44
    RET
.org 0x0817
@label_0817:
    TCMD cmd=0
    TXYR dx=0 dy=0
    TDEGA angle=2 dw=8
    TSPDA v=16 a=0
    TNUMA n=0 ns=0
    TTYPE type=0
    TCOL color=32
    JDIF easy=@label_083C norm=@label_084D hard=@label_084A luna=@label_0847
.org 0x083C
@label_083C:
    TNUMR n=2 ns=0
    TSPDR v=-4 a=0
    JMP jmp=@label_0850
.org 0x0847
@label_0847:
    TNUMR n=2 ns=0
.org 0x084A
@label_084A:
    TNUMR n=2 ns=0
.org 0x084D
@label_084D:
    TNUMR n=4 ns=0
.org 0x0850
@label_0850:
    NOP count=2
    TDEGR angle=4 dw=0
    PSE id=16
    TAMA2
    LOOP jmp=@label_0850 count=30
    TDEGA angle=128 dw=8
    TSPDR v=6 a=0
.org 0x0866
@label_0866:
    NOP count=2
    TDEGR angle=-4 dw=0
    PSE id=16
    TAMA2
    LOOP jmp=@label_0866 count=30
    TDEGA angle=2 dw=8
    TSPDR v=6 a=0
.org 0x087C
@label_087C:
    NOP count=2
    TDEGR angle=4 dw=0
    PSE id=16
    TAMA2
    LOOP jmp=@label_087C count=30
    RET
.org 0x088D
@label_088D:
    TCMD cmd=89
    TXYR dx=0 dy=0
    TDEGA angle=0 dw=0
    TSPDA v=0 a=4
    TNUMA n=50 ns=0
    TREP rep=130
    TTYPE type=1
    TCOL color=5
    TAUTO interval=16
    RET
.org 0x08A6
@label_08A6:
    TCMD cmd=2
    TXYR dx=0 dy=0
    TDEGA angle=0 dw=255
    TSPDA v=214 a=-3
    TNUMA n=12 ns=0
    TREP rep=70
    TTYPE type=1
    TCOL color=20
    TAUTO interval=8
    RET
.org 0x08BF
@label_08BF:
    TCMD cmd=1
    TXYR dx=0 dy=0
    TDEGA angle=0 dw=0
    TSPDA v=14 a=0
    TNUMA n=12 ns=0
    TTYPE type=0
    TCOL color=21
    RET
.org 0x08D4
@label_08D4:
    TCMD cmd=2
    TXYR dx=0 dy=0
    TDEGA angle=192 dw=100
    TSPDA v=208 a=3
    TREP rep=50
    TNUMA n=8 ns=0
    TTYPE type=7
    RET
.org 0x08E9
@label_08E9:
    TCMD cmd=1
    TXYR dx=0 dy=0
    TDEGA angle=0 dw=0
    TSPDA v=13 a=0
    TTYPE type=0
    TCOL color=5
    TDEGS
    JDIF easy=@label_090C norm=@label_0910 hard=@label_0914 luna=@label_0918
.org 0x090C
@label_090C:
    TNUMA n=3 ns=0
    RET
.org 0x0910
@label_0910:
    TNUMA n=6 ns=0
    RET
.org 0x0914
@label_0914:
    TNUMA n=7 ns=0
    RET
.org 0x0918
@label_0918:
    TNUMA n=8 ns=0
    RET
    RET
.org 0x091D
@label_091D:
    TCMD cmd=0
    TXYR dx=0 dy=0
    TDEGA angle=0 dw=0
    TREP rep=60
    TNUMA n=2 ns=0
    TTYPE type=1
    TCOL color=0
    RND reg=0
    MOD reg=0 div=2000
    CMPC reg=0 val=1000
    JL jmp=@label_0954
    MOVC dst=0 val=16
    MOVC dst=1 val=22
    JMP jmp=@label_0965
.org 0x0954
@label_0954:
    MOVC dst=0 val=112
    MOVC dst=1 val=4294967274
    JMP jmp=@label_0965
.org 0x0965
@label_0965:
    JDIF easy=@label_0976 norm=@label_0981 hard=@label_098C luna=@label_0997
.org 0x0976
@label_0976:
    MOVC dst=3 val=16
    JMP jmp=@label_09A2
.org 0x0981
@label_0981:
    MOVC dst=3 val=18
    JMP jmp=@label_09A2
.org 0x098C
@label_098C:
    MOVC dst=3 val=20
    JMP jmp=@label_09A2
.org 0x0997
@label_0997:
    MOVC dst=3 val=22
    JMP jmp=@label_09A2
.org 0x09A2
@label_09A2:
    TSPDA v=11 a=3
    MOVC dst=2 val=0
    MOVR dst=134 src=0
.org 0x09AE
@label_09AE:
    MOVR dst=135 src=2
    TAMA2
    TDEGR angle=-128 dw=0
    TAMA2
    TDEGR angle=-128 dw=0
    TSPDR v=-1 a=0
    INC reg=2
    INC reg=2
    CMPR reg0=2 reg1=3
    JS jmp=@label_09AE
    ADD dst=0 src=1
    PSE id=16
    NOP count=3
    PSE id=16
    NOP count=3
    LOOP jmp=@label_09A2 count=20
    TCOL color=1
.org 0x09DE
@label_09DE:
    TSPDA v=11 a=3
    MOVC dst=2 val=0
    MOVR dst=134 src=0
.org 0x09EA
@label_09EA:
    MOVR dst=135 src=2
    TAMA2
    TDEGR angle=-128 dw=0
    TAMA2
    TDEGR angle=-128 dw=0
    TSPDR v=-1 a=0
    INC reg=2
    INC reg=2
    CMPR reg0=2 reg1=3
    JS jmp=@label_09EA
    SUB dst=0 src=1
    PSE id=16
    NOP count=3
    PSE id=16
    NOP count=3
    LOOP jmp=@label_09DE count=20
    RET
.org 0x0A19
@label_0A19:
    TCMD cmd=5
    TXYR dx=0 dy=0
    TDEGA angle=0 dw=0
    TSPDA v=18 a=0
    TREP rep=50
    TNUMA n=16 ns=4
    TTYPE type=4
    TCOL color=33
    TDEGE
    RET
.org 0x0A31
@label_0A31:
    TCMD cmd=0
    TDEGA angle=0 dw=2
    TSPDA v=14 a=0
    TNUMA n=1 ns=1
    TTYPE type=0
    TCOL color=34
    TXYR dx=-80 dy=-40
    RET
.org 0x0A46
@label_0A46:
    TCMD cmd=4
    TSPDA v=8 a=6
    TNUMA n=1 ns=12
    TTYPE type=7
    TCOL color=0
    TXYR dx=0 dy=0
    MOVR dst=5 src=134
    MOVC dst=4 val=4294967232
    ADD dst=4 src=3
    MOVR dst=134 src=4
    TAMA2
    SUB dst=4 src=3
    SUB dst=4 src=3
    MOVR dst=134 src=4
    TAMA2
    MOVR dst=134 src=5
    RET
.org 0x0A75
@label_0A75:
    LCMD cmd=0
    LLA len=12800
    LNUMA n=3
    LSPDA v=448
    LCOL color=0
    LTYPE type=1
    LWA w=192
    LXY x=0 y=0
    LDEGA angle=10 dw=12
    LASER
    LDEGA angle=118 dw=12
    LASER
    PSE id=3
    RET
.org 0x0A9C
@label_0A9C:
    LNUMA n=1
    LSPDA v=64
    LCOL color=0
    LWA w=1280
    LTYPE type=0
    LDEGA angle=120 dw=0
    LXY x=-80 y=-40
    LLSET
    LDEGA angle=8 dw=0
    LXY x=80 y=-40
    LLSET
    RET
.org 0x0ABF
@label_0ABF:
    LNUMA n=1
    LSPDA v=192
    LCOL color=3
    LWA w=3840
    LDEGA angle=32 dw=0
    LXY x=0 y=0
    LTYPE type=0
    LLSET
    RET
.org 0x0AD9
@label_0AD9:
    LNUMA n=1
    LSPDA v=192
    LCOL color=3
    LWA w=3840
    LDEGA angle=96 dw=0
    LXY x=0 y=0
    LTYPE type=0
    LLSET
    RET
.org 0x0AF3
@label_0AF3:
    LLDEGR id=0 deg=-1
    LLDEGR id=1 deg=1
    RET
.org 0x0AFA
@label_0AFA:
    LLDEGR id=0 deg=1
    LLDEGR id=1 deg=-1
    RET
.org 0x0B01
@script_14:
    SETUP hp=15000 score=0
    DAMAGE_OFF
    HITSB_OFF
    CLIP_ON
    ANM pattern=10 speed=0
    DEGA angle=192
    SPDA speed=64
    ACC accel=2 count=40
    CLIP_OFF
.org 0x0B1C
@label_0B1C:
    ACC accel=2 count=1000
    JMP jmp=@label_0B1C
