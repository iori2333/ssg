.header 20
.offset 0 0x0054
.offset 1 0x00BB
.offset 2 0x010A
.offset 3 0x0153
.offset 4 0x018B
.offset 5 0x01BE
.offset 6 0x01F1
.offset 7 0x0233
.offset 8 0x0275
.offset 9 0x0356
.offset 10 0x039E
.offset 11 0x05F9
.offset 12 0x067E
.offset 13 0x084E
.offset 14 0x093C
.offset 15 0x0956
.offset 16 0x0963
.offset 17 0x0054
.offset 18 0x0054
.offset 19 0x0054

.org 0x0054
@script_0:
@script_17:  ; shared
@script_18:  ; shared
@script_19:  ; shared
    SETUP hp=150 score=3000
    RLCHG_ON
    ANM pattern=1 speed=0
    TCMD cmd=12
    TDEGA angle=0 dw=18
    TNUMA n=5 ns=2
    TSPDA v=8 a=0
    TTYPE type=0
    TCOL color=16
    TAUTO interval=255
.org 0x0072
@label_0072:
    DEGA angle=112
    SPDA speed=64
    MOV count=60
    SPDA speed=64
    LROL vx=-24 vy=24 deg=-1 count=256
    LOOP jmp=@label_0072 count=2
.org 0x0094
@label_0094:
    DEGA angle=16
    SPDA speed=64
    MOV count=60
    SPDA speed=64
    LROL vx=24 vy=24 deg=-1 count=256
    LOOP jmp=@label_0094 count=2
    JMP jmp=@label_0072
.org 0x00BB
@script_1:
    SETUP hp=180 score=5000
    RLCHG_ON
    CLIP_ON
    ANM pattern=0 speed=0
    TCMD cmd=1
    TDEGA angle=64 dw=0
    TNUMA n=4 ns=0
    TSPDA v=14 a=0
    TTYPE type=0
    TCOL color=5
    SPDA speed=128
    DEGA angle=64
    MOV count=100
    TAUTO interval=3
    NOP count=60
    CLIP_OFF
    TAUTO interval=0
.org 0x00EA
@label_00EA:
    DEGR angle=1
    NOP count=1
    LOOP jmp=@label_00EA count=31
    TAUTO interval=3
    TDEGR angle=32 dw=0
    NOP count=70
    TAUTO interval=0
    DEGR angle=-128
.org 0x0102
@label_0102:
    MOV count=100
    JMP jmp=@label_0102
.org 0x010A
@script_2:
    SETUP hp=15 score=500
    RLCHG_ON
    CLIP_ON
    ANM pattern=2 speed=0
    SPDA speed=256
    DEGA angle=96
    RND reg=0
    MOD reg=0 div=256
    CMPC reg=0 val=64
    JL jmp=@label_0143
    TCMD cmd=88
    TDEGA angle=0 dw=2
    TNUMA n=1 ns=0
    TSPDA v=8 a=0
    TTYPE type=0
    TCOL color=16
    TAUTO interval=255
.org 0x0143
@label_0143:
    MOV count=60
    CLIP_OFF
    ROL deg=2 count=32
.org 0x014B
@label_014B:
    MOV count=100
    JMP jmp=@label_014B
.org 0x0153
@script_3:
    SETUP hp=120 score=2000
    RLCHG_ON
    ANM pattern=4 speed=0
    SPDA speed=128
    DEGA angle=64
    TCMD cmd=1
    TDEGA angle=0 dw=0
    TNUMA n=13 ns=0
    TSPDA v=9 a=0
    TTYPE type=0
    TCOL color=17
    MOV count=80
    TAUTO interval=16
    ROL deg=4 count=64
    TAUTO interval=0
    DEGA angle=192
.org 0x0183
@label_0183:
    MOV count=100
    JMP jmp=@label_0183
.org 0x018B
@script_4:
    SETUP hp=30 score=600
    RLCHG_ON
    CLIP_ON
    ANM pattern=3 speed=0
    SPDA speed=384
    DEGA angle=96
.org 0x01A0
@label_01A0:
    MOV count=2
    SPDR speed=-16
    LOOP jmp=@label_01A0 count=30
    CLIP_OFF
    DEGS
.org 0x01B1
@label_01B1:
    MOV count=2
    SPDR speed=8
    JMP jmp=@label_01B1
.org 0x01BE
@script_5:
    SETUP hp=30 score=600
    RLCHG_ON
    CLIP_ON
    ANM pattern=3 speed=0
    SPDA speed=384
    DEGA angle=96
.org 0x01D3
@label_01D3:
    MOV count=2
    SPDR speed=-10
    LOOP jmp=@label_01D3 count=40
    CLIP_OFF
    DEGS
.org 0x01E4
@label_01E4:
    MOV count=2
    SPDR speed=8
    JMP jmp=@label_01E4
.org 0x01F1
@script_6:
    SETUP hp=14999 score=0
    RLCHG_ON
    CLIP_ON
    DAMAGE_OFF
    HITSB_OFF
    ANM pattern=5 speed=0
    SPDA speed=320
    DEGA angle=192
.org 0x0208
@label_0208:
    MOV count=2
    DEGR angle=-2
    SPDR speed=16
    LOOP jmp=@label_0208 count=8
.org 0x0219
@label_0219:
    MOV count=3
    DEGR angle=1
    SPDR speed=16
    LOOP jmp=@label_0219 count=30
    CLIP_OFF
.org 0x022B
@label_022B:
    MOV count=100
    JMP jmp=@label_022B
.org 0x0233
@script_7:
    SETUP hp=14999 score=0
    RLCHG_ON
    CLIP_ON
    DAMAGE_OFF
    HITSB_OFF
    ANM pattern=6 speed=0
    SPDA speed=320
    DEGA angle=192
.org 0x024A
@label_024A:
    MOV count=2
    DEGR angle=-2
    SPDR speed=16
    LOOP jmp=@label_024A count=8
.org 0x025B
@label_025B:
    MOV count=3
    DEGR angle=1
    SPDR speed=16
    LOOP jmp=@label_025B count=30
    CLIP_OFF
.org 0x026D
@label_026D:
    MOV count=100
    JMP jmp=@label_026D
.org 0x0275
@script_8:
    SETUP hp=1800 score=1000000
    ANM pattern=7 speed=0
    ANMEX pattern=12
    ITEM type=2
    STI jmp=@label_0314 vector=HP val=250
    DAMAGE_OFF
    MOV count=180
    NOP count=20
    DAMAGE_ON
.org 0x0297
@label_0297:
    SPDA speed=64
    CALL jmp=@script_9
.org 0x02A1
@label_02A1:
    TAMA
    NOP count=3
    TDEGR angle=2 dw=0
    LOOP jmp=@label_02A1 count=10
    NOP count=20
    TDEGR angle=-10 dw=0
.org 0x02B5
@label_02B5:
    TAMA
    NOP count=3
    TDEGR angle=-2 dw=0
    LOOP jmp=@label_02B5 count=10
    NOP count=10
    TDEGR angle=10 dw=0
    NOP count=50
    DEGS
    SPDA speed=256
    MOV count=15
    CALL jmp=@label_0368
    MOV count=15
    DEGR angle=-128
    MXYA x=319 y=110 count=30
    CALL jmp=@label_037A
    DEGA angle=0
    WAVX vx=96 amp=40 vd=-4 count=64
    TCOL color=5
    WAVX vx=-96 amp=40 vd=-4 count=128
    TAUTO interval=0
    WAVX vx=96 amp=40 vd=-4 count=64
    NOP count=60
    JMP jmp=@label_0297
.org 0x0314
@label_0314:
    CLI vector=HP
    CALL jmp=@label_038C
    MOVC dst=0 val=1
    MOVR dst=1 src=0
    TCLR
    MXYA x=319 y=110 count=30
.org 0x032C
@label_032C:
    MOVR dst=2 src=0
    ADD dst=2 src=0
    MOVR dst=136 src=2
    NOP count=3
    TAMA
    TDEGR angle=5 dw=0
    LOOP jmp=@label_032C count=20
    CMPC reg=0 val=3
    JL jmp=@label_032C
    ADD dst=0 src=1
    JMP jmp=@label_032C
.org 0x0356
@script_9:
    TCMD cmd=1
    TDEGA angle=0 dw=0
    TNUMA n=10 ns=0
    TSPDA v=20 a=-6
    TREP rep=20
    TTYPE type=1
    TCOL color=34
    RET
.org 0x0368
@label_0368:
    TCMD cmd=0
    TDEGA angle=64 dw=12
    TNUMA n=18 ns=0
    TSPDA v=10 a=0
    TTYPE type=0
    TCOL color=16
    TAUTO interval=4
    RET
.org 0x037A
@label_037A:
    TCMD cmd=6
    TDEGA angle=64 dw=96
    TNUMA n=2 ns=6
    TSPDA v=6 a=0
    TTYPE type=0
    TCOL color=0
    TAUTO interval=24
    RET
.org 0x038C
@label_038C:
    TCMD cmd=80
    TDEGA angle=64 dw=16
    TNUMA n=2 ns=0
    TSPDA v=17 a=-6
    TTYPE type=1
    TREP rep=20
    TCOL color=1
    RET
.org 0x039E
@script_10:
    SETUP hp=7500 score=6000000
    ANM pattern=8 speed=0
    STI jmp=@label_0510 vector=BOSSLEFT val=1
    STI jmp=@label_045D vector=HP val=4500
    STI jmp=@label_045D vector=TIMER val=5000
    DEGA angle=64
    SPDA speed=256
    MOVC dst=5 val=2
    LDEGA angle=0 dw=0
    LNUMA n=1
    LSPDA v=64
    LCOL color=0
    LWA w=2560
    CLIP_ON
    DAMAGE_OFF
    NOP count=105
    WAVY vy=202 amp=90 vd=2 count=80
    NOP count=20
    DAMAGE_ON
.org 0x03F8
@label_03F8:
    MXYA x=319 y=139 count=60
    DEGA angle=0
    ANM pattern=8 speed=0
    ANMEX pattern=10
    SPDA speed=192
.org 0x040B
@label_040B:
    CALL jmp=@script_11
    ROL deg=4 count=4
    LOOP jmp=@label_040B count=31
.org 0x041B
@label_041B:
    CALL jmp=@label_0612
    ROL deg=-4 count=4
    LOOP jmp=@label_041B count=31
    DEGA angle=192
.org 0x042D
@label_042D:
    MOV count=1
    SPDR speed=3
    LOOP jmp=@label_042D count=60
    NOP count=20
    DEGS
    CALL jmp=@label_063E
    PSE id=14
.org 0x0447
@label_0447:
    MOV count=1
    SPDR speed=3
    LOOP jmp=@label_0447 count=70
    TAUTO interval=0
    JMP jmp=@label_03F8
.org 0x045D
@label_045D:
    SETUP hp=4500 score=6000000
    STI jmp=@label_0523 vector=HP val=675
    STI jmp=@label_0523 vector=TIMER val=5000
    TCLR
.org 0x047B
@label_047B:
    MXYA x=319 y=139 count=60
    DEGA angle=0
    NOP count=20
    RND reg=0
    MOD reg=0 div=120
    CMPC reg=0 val=60
    JS jmp=@label_04BC
    JMP jmp=@label_049F
.org 0x049F
@label_049F:
    CALL jmp=@label_0651
.org 0x04A4
@label_04A4:
    TDEGR angle=6 dw=0
    NOP count=7
    PSE id=12
    TAMA
    LOOP jmp=@label_04A4 count=7
    NOP count=100
    JMP jmp=@label_04D9
.org 0x04BC
@label_04BC:
    CALL jmp=@label_0651
.org 0x04C1
@label_04C1:
    TDEGR angle=-6 dw=0
    NOP count=7
    PSE id=12
    TAMA
    LOOP jmp=@label_04C1 count=7
    NOP count=100
    JMP jmp=@label_04D9
.org 0x04D9
@label_04D9:
    SPDA speed=192
    DEGA angle=192
.org 0x04E0
@label_04E0:
    MOV count=1
    SPDR speed=3
    LOOP jmp=@label_04E0 count=60
    NOP count=20
    DEGS
    CALL jmp=@label_062B
    PSE id=14
.org 0x04FA
@label_04FA:
    MOV count=1
    SPDR speed=3
    LOOP jmp=@label_04FA count=70
    TAUTO interval=0
    JMP jmp=@label_047B
.org 0x0510
@label_0510:
    MOVC dst=5 val=1
    TCLR
    CLI vector=BOSSLEFT
    STI jmp=@label_05E5 vector=TIMER val=4000
.org 0x0523
@label_0523:
    SETUP hp=675 score=6000000
    CLI vector=HP
    CMPC reg=5 val=1
    JEQ jmp=@label_0543
    STI jmp=@label_05E5 vector=TIMER val=4000
.org 0x0543
@label_0543:
    TCLR
    MXYA x=319 y=40 count=60
    TREP rep=2
    MOVC dst=0 val=0
    MOVC dst=4 val=25
    MOVC dst=2 val=128
    MOVC dst=1 val=2
    MOVC dst=3 val=3
    JDIF easy=@label_057C norm=@label_057F hard=@label_0582 luna=@label_0585
.org 0x057C
@label_057C:
    ADD dst=1 src=3
.org 0x057F
@label_057F:
    ADD dst=1 src=3
.org 0x0582
@label_0582:
    ADD dst=1 src=3
.org 0x0585
@label_0585:
    ADD dst=1 src=3
.org 0x0588
@label_0588:
    CALL jmp=@label_066D
    CMPC reg=5 val=1
    JL jmp=@label_05DD
    CALL jmp=@label_0660
    MOVR dst=6 src=4
.org 0x05A0
@label_05A0:
    MOVR dst=142 src=0
    TAMA2
    ADD dst=0 src=2
    MOVR dst=142 src=0
    TAMA2
    ADD dst=0 src=2
    ADD dst=0 src=1
    NOP count=5
    DEC reg=6
    CMPC reg=6 val=1
    JL jmp=@label_05A0
    DEC reg=4
    DEC reg=4
    DEC reg=4
    CMPC reg=4 val=8
    JL jmp=@label_0588
    MOVC dst=4 val=8
    JMP jmp=@label_0588
.org 0x05DD
@label_05DD:
    NOP count=50
    JMP jmp=@label_0588
.org 0x05E5
@label_05E5:
    SETUP hp=0 score=0  ; death marker
    CLI vector=TIMER
    TCLR
    NOP count=100
    JMP jmp=@label_05E5
.org 0x05F9
@script_11:
    TCMD cmd=0
    TDEGA angle=0 dw=1
    TDEGE
    TDEGR angle=-64 dw=0
    TNUMA n=3 ns=1
    TSPDA v=1 a=5
    TREP rep=40
    TTYPE type=1
    TCOL color=16
    PSE id=4
    TAMA
    RET
.org 0x0612
@label_0612:
    TCMD cmd=0
    TDEGA angle=0 dw=1
    TDEGE
    TDEGR angle=64 dw=0
    TNUMA n=3 ns=1
    TSPDA v=1 a=5
    TREP rep=40
    TTYPE type=1
    TCOL color=16
    PSE id=4
    TAMA
    RET
.org 0x062B
@label_062B:
    TCMD cmd=2
    TDEGA angle=0 dw=90
    TDEGE
    TNUMA n=1 ns=1
    TSPDA v=15 a=0
    TTYPE type=0
    TCOL color=3
    TAUTO interval=4
    RET
.org 0x063E
@label_063E:
    TCMD cmd=0
    TDEGA angle=0 dw=10
    TDEGE
    TNUMA n=1 ns=1
    TSPDA v=6 a=0
    TTYPE type=0
    TCOL color=3
    TAUTO interval=8
    RET
.org 0x0651
@label_0651:
    TCMD cmd=1
    TNUMA n=12 ns=1
    TSPDA v=12 a=-4
    TREP rep=100
    TTYPE type=1
    TCOL color=21
    RET
.org 0x0660
@label_0660:
    TCMD cmd=9
    TNUMA n=10 ns=1
    TSPDA v=17 a=0
    TTYPE type=8
    TCOL color=3
    RET
.org 0x066D
@label_066D:
    TCMD cmd=12
    TDEGA angle=0 dw=10
    TNUMA n=11 ns=2
    TSPDA v=8 a=0
    TTYPE type=0
    TCOL color=21
    TAMA
    RET
.org 0x067E
@script_12:
    SETUP hp=6750 score=6000000
    ANM pattern=9 speed=0
    ANMEX pattern=11
    STI jmp=@label_07FD vector=BOSSLEFT val=1
    STI jmp=@label_07D8 vector=HP val=675
    STI jmp=@label_07D8 vector=TIMER val=10000
    DEGA angle=128
    SPDA speed=320
    CLIP_ON
    DAMAGE_OFF
    LROL vx=0 vy=-128 deg=-1 count=60
    DEGA angle=64
    SPDA speed=224
.org 0x06C6
@label_06C6:
    DEGR angle=-7
    MOV count=8
    LOOP jmp=@label_06C6 count=10
    DAMAGE_ON
.org 0x06D3
@label_06D3:
    JHPS hp=1766 jmp=0x0DAC
    CALL jmp=@label_0874
    JMP jmp=@label_06F1
    RLCHG_ON
    DEGA angle=160
    MOV count=25
    CALL jmp=@label_08F8
.org 0x06F1
@label_06F1:
    PSE id=1
    NOP count=30
    MOVR dst=0 src=143
    CMPC reg=0 val=20416
    JL jmp=@label_0710
    MXYA x=138 y=30 count=20
    JMP jmp=@label_0717
.org 0x0710
@label_0710:
    MXYA x=501 y=30 count=20
.org 0x0717
@label_0717:
    LDEGA angle=64 dw=0
    LNUMA n=1
    LSPDA v=64
    LCOL color=2
    LWA w=1280
    LTYPE type=0
    LLSET
    NOP count=150
    LLOPEN id=0
    RLCHG_ON
    NOP count=10
    DEGA angle=112
    SPDA speed=320
    MOV count=35
    LLCLOSE id=0
    MOV count=35
    NOP count=50
    JHPL hp=1747 jmp=0x0DAC
    LNUMA n=1
    LSPDA v=64
    LCOL color=3
    LWA w=1280
    LDEGS
    LTYPE type=0
    MOVR dst=0 src=128
    CMPC reg=0 val=128
    JL jmp=@label_06D3
    PSE id=1
    MOVR dst=0 src=143
    CMPC reg=0 val=20416
    JL jmp=@label_07AB
    LDEGR angle=21 dw=0
    LLSET
    NOP count=140
.org 0x0785
@label_0785:
    LLDEGR id=0 deg=-1
    NOP count=3
    LOOP jmp=@label_0785 count=20
    LLOPEN id=0
.org 0x0794
@label_0794:
    LLDEGR id=0 deg=-1
    NOP count=4
    LOOP jmp=@label_0794 count=25
    LLCLOSE id=0
    NOP count=50
    JMP jmp=@label_06D3
.org 0x07AB
@label_07AB:
    LDEGR angle=-21 dw=0
    LLSET
    NOP count=140
.org 0x07B2
@label_07B2:
    LLDEGR id=0 deg=1
    NOP count=3
    LOOP jmp=@label_07B2 count=20
    LLOPEN id=0
.org 0x07C1
@label_07C1:
    LLDEGR id=0 deg=1
    NOP count=4
    LOOP jmp=@label_07C1 count=25
    LLCLOSE id=0
    NOP count=50
    JMP jmp=@label_06D3
.org 0x07D8
@label_07D8:
    CLI vector=HP
    STI jmp=@label_083A vector=TIMER val=2400
    SETUP hp=675 score=6000000
    TCLR
    LLCLOSE id=255
.org 0x07F0
@label_07F0:
    CALL jmp=@label_0861
    NOP count=70
    JMP jmp=@label_07F0
.org 0x07FD
@label_07FD:
    CLI vector=HP
    CLI vector=BOSSLEFT
    STI jmp=@label_083A vector=TIMER val=2400
    SETUP hp=675 score=6000000
    TCLR
    LLCLOSE id=255
    MXYA x=319 y=79 count=60
.org 0x081E
@label_081E:
    CALL jmp=@script_13
    NOP count=30
    LOOP jmp=@label_081E count=4
    NOP count=50
    CALL jmp=@label_091E
    JMP jmp=@label_081E
.org 0x083A
@label_083A:
    CLI vector=TIMER
    SETUP hp=0 score=0  ; death marker
    TCLR
    NOP count=100
    JMP jmp=@label_083A
.org 0x084E
@script_13:
    TCMD cmd=4
    TDEGA angle=192 dw=16
    TNUMA n=10 ns=2
    TSPDA v=18 a=-6
    TREP rep=1
    TTYPE type=2
    TCOL color=17
    TAMA
    RET
.org 0x0861
@label_0861:
    TCMD cmd=1
    TDEGA angle=192 dw=12
    TNUMA n=8 ns=1
    TSPDA v=16 a=-10
    TREP rep=1
    TTYPE type=2
    TCOL color=17
    TAMA
    RET
.org 0x0874
@label_0874:
    TCMD cmd=4
    TNUMA n=1 ns=1
    TSPDA v=16 a=0
    TTYPE type=0
    TCOL color=17
    TDEGA angle=0 dw=1
    TDEGS
    JDIF easy=@label_089E norm=@label_089B hard=@label_0898 luna=@label_0895
.org 0x0895
@label_0895:
    TNUMR n=0 ns=1
.org 0x0898
@label_0898:
    TNUMR n=0 ns=1
.org 0x089B
@label_089B:
    TNUMR n=0 ns=1
.org 0x089E
@label_089E:
    MOVC dst=0 val=65
    MOVR dst=1 src=134
.org 0x08A7
@label_08A7:
    NOP count=2
    PSE id=16
    MOVR dst=2 src=1
    ADD dst=2 src=0
    MOVR dst=134 src=2
    TAMA2
    MOVR dst=2 src=1
    SUB dst=2 src=0
    MOVR dst=134 src=2
    TAMA2
    DEC reg=0
    DEC reg=0
    DEC reg=0
    DEC reg=0
    LOOP jmp=@label_08A7 count=13
.org 0x08CF
@label_08CF:
    NOP count=2
    PSE id=16
    MOVR dst=2 src=1
    ADD dst=2 src=0
    MOVR dst=134 src=2
    TAMA2
    MOVR dst=2 src=1
    SUB dst=2 src=0
    MOVR dst=134 src=2
    TAMA2
    INC reg=0
    INC reg=0
    INC reg=0
    INC reg=0
    LOOP jmp=@label_08CF count=20
    RET
.org 0x08F8
@label_08F8:
    TCMD cmd=1
    TDEGA angle=0 dw=12
    TNUMA n=6 ns=1
    TSPDA v=16 a=0
    TTYPE type=0
    TCOL color=1
.org 0x0907
@label_0907:
    PSE id=16
    TAMA
    NOP count=2
    PSE id=16
    TAMA
    NOP count=4
    TDEGR angle=8 dw=0
    LOOP jmp=@label_0907 count=12
    RET
.org 0x091E
@label_091E:
    LCMD cmd=8
    LLA len=16000
    LDEGA angle=0 dw=16
    LNUMA n=3
    LSPDA v=512
    LCOL color=0
    LTYPE type=0
    LWA w=192
    LASER
    PSE id=3
    RET
.org 0x093C
@script_14:
    SETUP hp=149999 score=0
    DEGA angle=64
    HITSB_OFF
    DAMAGE_OFF
    CLIP_ON
    MOV count=100
    CLIP_OFF
.org 0x094E
@label_094E:
    MOV count=100
    JMP jmp=@label_094E
.org 0x0956
@script_15:
    ANM pattern=14 speed=0
    SPDA speed=512
    JMP jmp=@script_14
.org 0x0963
@script_16:
    TCMD cmd=1
    TNUMA n=7 ns=1
    TSPDA v=12 a=0
    TREP rep=30
    TVDEG vd=1
    TTYPE type=4
    TCOL color=32
    TAMA
    RET
    TCMD cmd=1
    TNUMA n=7 ns=1
    TSPDA v=17 a=0
    TREP rep=30
    TVDEG vd=-2
    TTYPE type=4
    TCOL color=32
    TAMA
    RET
    TCMD cmd=1
    TNUMA n=7 ns=1
    TSPDA v=22 a=0
    TREP rep=30
    TVDEG vd=3
    TTYPE type=4
    TCOL color=32
    TAMA
    RET
