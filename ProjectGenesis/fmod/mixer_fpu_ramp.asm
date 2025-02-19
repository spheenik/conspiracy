;to compile:
;
;nasm -f elf hello.asm
;ld -s -o hello hello.o

%define VOLUMERAMPING
; %define ROLLED

%define FSOUND_MIXDIR_FORWARDS		1
%define FSOUND_MIXDIR_BACKWARDS		2

%define FSOUND_OUTPUTBUFF_END		0
%define FSOUND_SAMPLEBUFF_END		1
%define FSOUND_VOLUMERAMP_END		2

%define FSOUND_LOOP_NORMAL	0x02		; For forward looping samples.
%define FSOUND_LOOP_BIDI	0x04		; For bidirectional looping samples.  (no effect if in hardware).


; Channel type - contains information on a mixing channel
STRUC Channel
	.index: RESD 1		    ; position in channel pool.
	.volume: RESD 1  		; current volume (00-FFh).
	.frequency: RESD 1		; speed or rate of playback in hz.
	.pan: RESD 1   			; panning value (00-FFh).
	.actualvolume: RESD 1   ; driver level current volume.
	.actualpan: RESD 1   	; driver level panning value.
	.sampleoffset: RESD 1 	; sample offset (sample starts playing from here).

	.sptr: RESD 1			; currently playing sample

	; software mixer stuff
	.leftvolume: RESD 1      ; mixing information. adjusted volume for left channel (panning involved)
	.rightvolume: RESD 1     ; mixing information. adjusted volume for right channel (panning involved)
	.mixpos: RESD 1			 ; mixing information. high part of 32:32 fractional position in sample
	.mixposlo: RESD 1		 ; mixing information. low part of 32:32 fractional position in sample
	.speedlo: RESD 1		 ; mixing information. playback rate - low part fractional
	.speedhi: RESD 1		 ; mixing information. playback rate - high part fractional
	.speeddir: RESD 1		 ; mixing information. playback direction - forwards or backwards

	; software mixer volume ramping stuff
	.ramp_lefttarget: RESD 1
	.ramp_righttarget: RESD 1
	.ramp_leftvolume: RESD 1
	.ramp_rightvolume: RESD 1
	.ramp_leftspeed: RESD 1
	.ramp_rightspeed: RESD 1
	.ramp_count: RESD 1
	.size:
ENDSTRUC


STRUC Sample
    .buff: RESD 1		    ;	0   unsigned char *; pointer to sound data

	; ====================================================================================
	; BUGFIX 1.5 (finetune was in wrong place)
	; DONT CHANGE THE ORDER OF THESE NEXT MEMBERS AS THEY ARE ALL LOADED IN A SINGLE READ

    .length: RESD 1		    ;	4   unsigned int  sample length (samples)
    .loopstart: RESD 1		;	8   unsigned int  sample loop start (samples)
    .looplen: RESD 1		;	12  unsigned int  sample loop length (samples)
    .defvol: RESB 1		    ;	16  unsigned char sample default volume
    .finetune: RESB 1		;	17  signed char	 finetune value from -128 to 127

    RESB 2                  ;   18

	; ====================================================================================

    .deffreq: RESD 1		;	20  int 		 sample default speed in hz
    .defpan: RESD 1		    ;	24  int			 sample default pan

    .bitsx: RESB 1    	    ;	28  unsigned char bits per sample
    .loopmode: RESB 1	    ;  	29  unsigned char loop type

	; music stuff
    .globalvol: RESB 1	    ;	30  unsigned char  sample global volume (scalar)
    .relative: RESB 1	    ;	31  signed char	 relative note
    .middlec: RESD 1		;	32  int			 finetuning adjustment to make for music samples.. relative to 8363hz
    .susloopbegin: RESD 1	;	36  unsigned int  sample loop start
    .susloopend: RESD 1		;	40  unsigned int  sample loop length
    .vibspeed: RESB 1		;	44  unsigned char  vibrato speed 0-64
    .vibdepth: RESB 1		;	45  unsigned char vibrato depth 0-64
    .vibtype: RESB 1		;	46  unsigned char vibrato type 0=sine, 1=rampdown, 2=square, 3=random
    .vibrate: RESB 1		;   47 unsigned char vibrato rate 0-64 (like sweep?)
ENDSTRUC


section .data

            global mix_volumerampsteps, mix_1overvolumerampsteps

cptr:           dd 0;
count:          dd 0;

mix_numsamples: dd 0;	// number of samples to mix
mix_mixbuffend: dd 0;
mix_mixptr:     dd 0;
mix_mixbuffptr: dd 0;
mix_endflag:    dd 0;
mix_sptr:       dd 0;
mix_cptr:       dd 0;
mix_count:      dd 0;
mix_samplebuff: dd 0;
mix_leftvol:    dd 0.0;
mix_rightvol:   dd 0.0;
mix_temp1:      dd 0;

mix_count_old:      dd 0;
mix_rampleftvol:    dd 0;
mix_ramprightvol:   dd 0;
mix_rampcount:      dd 0;
mix_rampspeedleft:  dd 0;
mix_rampspeedright: dd 0;

mix_volumerampsteps:        dd 0;
mix_1overvolumerampsteps:   dd 0;

mix_255:        dd 255.0;
mix_256:        dd 256.0;
mix_1over255:   dd 0.00392156862745098039    ; 1.0f / 255.0f;
mix_1over256:   dd 0.00390625                ; 1.0f / 256.0f;
mix_1over2gig:  dd 0.00000000046566128731    ; 1.0f / 2147483648.0f;

conv_temp: dd 0

section	.text

            global FSOUND_Mixer_FPU_Ramp

FSOUND_Mixer_FPU_Ramp:

			push	ebp
			mov     ebp, esp

			pushad

            mov     eax, [ebp + 8]              ; FSOUND_Channel array
            mov     [cptr], eax

			mov		ecx, [ebp + 12]              ; mixptr
			mov		[mix_mixptr], ecx

            mov     ebx, [ebp + 16]             ; len
            mov     [mix_numsamples], ebx

			cmp		ebx, 0
			jle		MixFinished

            shl     ebx, 3
            add     ebx, ecx
            mov     [mix_mixbuffend], ebx

            mov     dword [count], 64

        MixChannelStart:

            mov     ecx, [mix_mixptr]
			mov		[mix_mixbuffptr], ecx

			mov		ecx, [cptr]
			mov		[mix_cptr], ecx

			cmp		ecx, 0						; if (!cptr) ...
			je		MixExit						;			  ... then skip this channel!

			mov		ebx, [ecx + Channel.sptr]	; load the correct SAMPLE  pointer for this channel
			mov		[mix_sptr], ebx				; store sample pointer away
			cmp		ebx, 0						; if (!sptr) ...
			je		MixExit						;			  ... then skip this channel!

			; get pointer to sample buffer
			mov		eax, [ebx + Sample.buff]
			mov		[mix_samplebuff], eax

			;==============================================================================================
			; LOOP THROUGH CHANNELS
			; through setup code:- usually ebx = sample pointer, ecx = channel pointer
			;==============================================================================================

			;= SUCCESS - SETUP CODE FOR THIS CHANNEL ======================================================

			; =========================================================================================
			; the following code sets up a mix counter. it sees what will happen first, will the output buffer
			; end be reached first? or will the end of the sample be reached first? whatever is smallest will
			; be the mixcount.

			; first base mixcount on size of OUTPUT BUFFER (in samples not bytes)
			mov		eax, [mix_numsamples]

		CalculateLoopCount:

			mov		[mix_count], eax
			mov		esi, [ecx + Channel.mixpos]
			mov		ebp, FSOUND_OUTPUTBUFF_END
			mov		[mix_endflag], ebp			; set a flag to say mixing will end when end of output buffer is reached

			cmp		dword [ecx + Channel.speeddir], FSOUND_MIXDIR_FORWARDS
			jne		samplesleftbackwards

			; work out how many samples left from mixpos to loop end
			mov		edx, [ebx + Sample.loopstart]
			add		edx, [ebx + Sample.looplen]
			sub		edx, esi					; eax = samples left (loopstart+looplen-mixpos)
			mov		eax, [ecx + Channel.mixposlo]
			xor		ebp, ebp
			sub		ebp, eax
			sbb		edx, 0
			mov		eax, ebp
			jmp		samplesleftfinish

		samplesleftbackwards:
			; work out how many samples left from mixpos to loop start
			mov		edx, [ecx + Channel.mixpos]
			mov		eax, [ecx + Channel.mixposlo]

			sub		eax, 0h
			sbb		edx, [ebx + Sample.loopstart]

		samplesleftfinish:

			; edx:eax now contains number of samples left to mix
			cmp		edx, 1000000h
			jae		staywithoutputbuffend

			shrd	eax, edx, 8
			shr		edx, 8

			; now samples left = EDX:EAX -> hhhhhlll
			mov		ebp, [ecx + Channel.speedhi]
			mov		edi, [ecx + Channel.speedlo]
			shl		ebp, 24
			shr		edi, 8
			and		edi, 000FFFFFFh
			or		ebp, edi
			div		ebp

			or		edx,edx						; if fractional 16-bit part is zero we must add an extra carry number
			jz		dontaddbyte					;  to the resultant in EDX:EAX.
			inc		eax
		dontaddbyte:							; we must remove the fractional part of the multiply by shifting EDX:EAX
			cmp		eax, [mix_count]
			ja		staywithoutputbuffend
			mov		[mix_count], eax
			mov		edx, FSOUND_SAMPLEBUFF_END	; set a flag to say mix will end when end of output buffer is reached
			mov		[mix_endflag], edx
		staywithoutputbuffend:

			mov		ecx, [mix_cptr]

			;= VOLUME RAMP SETUP =========================================================
			; Reasons to ramp
			; 1. volume change
			; 2. sample starts (just treat as volume change - 0 to volume)
			; 3. sample ends (ramp last n number of samples from volume to 0)

			; now if the volume has changed, make end condition equal a volume ramp
			mov		dword [mix_rampspeedleft], 0		; clear out volume ramp
			mov		dword [mix_rampspeedright], 0		; clear out volume ramp

%ifdef VOLUMERAMPING
			mov		eax, [mix_count]
			mov		[mix_count_old], eax			; remember mix count before modifying it

			mov		dword [mix_rampcount], 0
			cmp		dword [ecx + Channel.ramp_count], 0
			je		volumerampstart

			; if it tries to continue an old ramp, but the target has changed,
			; set up a new ramp
			mov		eax, [ecx + Channel.leftvolume]
			mov		edx, [ecx + Channel.ramp_lefttarget]
			cmp		eax,edx
			jne		volumerampstart
			mov		eax, [ecx + Channel.rightvolume]
			mov		edx, [ecx + Channel.ramp_righttarget]
			cmp		eax,edx
			jne		volumerampstart

			; restore old ramp
			mov		eax, [ecx + Channel.ramp_count]
			mov		[mix_rampcount], eax
			mov		eax, [ecx + Channel.ramp_leftspeed]
			mov		[mix_rampspeedleft], eax
			mov		eax, [ecx + Channel.ramp_rightspeed]
			mov		[mix_rampspeedright], eax

			jmp		novolumerampR

		volumerampstart:
			mov		eax, [ecx + Channel.leftvolume]
			mov		edx, [ecx + Channel.ramp_leftvolume]
			shr		edx, 8
			mov		[ecx + Channel.ramp_lefttarget], eax
			sub		eax, edx
			cmp		eax, 0
			je		novolumerampL

			mov		[mix_temp1], eax
			fild	dword [mix_temp1]
			fmul	dword [mix_1over255]
			fmul	dword [mix_1overvolumerampsteps]
			fstp	dword [mix_rampspeedleft]
			mov		eax, [mix_rampspeedleft]
			mov		[ecx + Channel.ramp_leftspeed], eax
			mov		eax, [mix_volumerampsteps]
			mov		[mix_rampcount], eax


		novolumerampL:
			mov		eax, [ecx + Channel.rightvolume]
			mov		edx, [ecx + Channel.ramp_rightvolume]
			shr		edx, 8
			mov		[ecx + Channel.ramp_righttarget], eax
			sub		eax, edx
			cmp		eax, 0
			je		novolumerampR

			mov		[mix_temp1], eax
			fild	dword [mix_temp1]
			fmul	dword [mix_1over255]
			fmul	dword [mix_1overvolumerampsteps]
			fstp	dword [mix_rampspeedright]
			mov		eax, [mix_rampspeedright]
			mov		[ecx + Channel.ramp_rightspeed], eax
			mov		eax, [mix_volumerampsteps]
			mov		[mix_rampcount], eax


		novolumerampR:

			mov		eax, [mix_rampcount]
			cmp		eax, 0
			jle		volumerampend

			mov		[ecx + Channel.ramp_count], eax

			cmp		[mix_count], eax
			jbe		volumerampend	; dont clamp mixcount
			mov		[mix_count], eax
		volumerampend:
%endif


			;= SET UP VOLUME MULTIPLIERS ==================================================

			; set up left/right volumes
			mov		ecx, [mix_cptr]


			; right volume
			mov		eax, [ecx + Channel.rightvolume]
			mov		[mix_temp1], eax
			fild	dword [mix_temp1]
			fmul	dword [mix_1over255]
			fstp	dword [mix_rightvol]

			; left volume
			mov		eax, [ecx + Channel.leftvolume]
			mov		[mix_temp1], eax
			fild	dword [mix_temp1]
			fmul	dword [mix_1over255]
			fstp	dword [mix_leftvol]

			; right ramp volume
			mov		eax, [ecx + Channel.ramp_rightvolume]
			mov		[mix_temp1], eax
			fild	dword [mix_temp1]
			fmul	dword [mix_1over256]			; first convert from 24:8 to 0-255
			fmul	dword [mix_1over255]			; now make 0-1.0f
			fstp	dword [mix_ramprightvol]

			; left ramp volume
			mov		eax, [ecx + Channel.ramp_leftvolume]
			mov		[mix_temp1], eax
			fild	dword [mix_temp1]
			fmul	dword [mix_1over256]			; first convert from 24:8 to 0-255
			fmul	dword [mix_1over255]			; now make 0-1.0f
			fstp	dword [mix_rampleftvol]


			;= SET UP ALL OF THE REGISTERS HERE FOR THE INNER LOOP ====================================
			; eax = ---
			; ebx = speed low
			; ecx = speed high
			; edx = counter
			; esi = mixpos
			; edi = destination pointer
			; ebp = mixpos low

			mov		eax, [mix_cptr]
			mov		ebx, [eax + Channel.speedlo]
			mov		ecx, [eax + Channel.speedhi]
		;  mov		edx, [mix_count]
			mov		ebp, [eax + Channel.mixposlo]
			mov		esi, [eax + Channel.mixpos]
			mov		edi, [mix_mixbuffptr]		; point edi to 16bit output stream

			cmp		dword [eax + Channel.speeddir], FSOUND_MIXDIR_FORWARDS
			je		NoChangeSpeed
			xor		ebx, 0FFFFFFFFh
			xor		ecx, 0FFFFFFFFh
			add		ebx, 1
			adc		ecx, 0
		NoChangeSpeed:


			;======================================================================================
			; ** 16 BIT NORMAL FUNCTIONS **********************************************************
			;======================================================================================

			mov		eax, [mix_samplebuff]
			shr		eax, 1
			add		esi, eax

			mov		edx, [mix_count]

	%ifdef VOLUMERAMPING
			cmp		dword [mix_rampcount], 0
			jne		MixLoopStart16_2
	%endif

	%ifdef ROLLED
			jmp		MixLoopStart16
	%endif
			shr		edx, 1
			or		edx, edx
			jz		MixLoopStart16			; no groups of 2 samples to mix!

; START

			shr		ebp, 1					; 1 make 31bit coz fpu only loads signed values
			add		edi, 16					;
			fild	word [esi+esi+2]	    ; 1 [0]samp1+1
			mov		[mix_temp1], ebp		; 1
;			nop
			fild	word [esi+esi]		    ; 1 [0]samp1 [1]samp1+1
			fild	dword [mix_temp1]		; 1 [0]ifrac1 [1]samp1 [2]samp1+1
			add		ebp, ebp				; 1
;			nop
			add		ebp, ebx				; 1
			adc		esi, ecx				; 1
			fmul	dword [mix_1over2gig]	; 1 [0]ffrac1 [1]samp1 [2]samp1+1
			fild	word [esi+esi+2]	    ; 1 [0]samp2+1 [1]ffrac1 [2]samp1 [3]samp1+1
			shr		ebp, 1					; 1
;			nop
			mov		[mix_temp1], ebp		; 1
;			nop
			fild	dword [mix_temp1]		; 1 [0]ifrac2 [1]samp2+1 [2]ffrac1 [3]samp1 [4]samp1+1
			fild	word [esi+esi]		    ; 1 [0]samp2 [1]ifrac2 [2]samp2+1 [3]ffrac1 [4]samp1 [5]samp1+1
			fxch	st5					    ;   [0]samp1+1 [1]ifrac2 [2]samp2+1 [3]ffrac1 [4]samp1 [5]samp2
			fsub	st0, st4			    ; 1 [0]delta1 [1]ifrac2 [2]samp2+1 [3]ffrac1 [4]samp1 [5]samp2
;			fnop							; 1 fsub stall
			shl		ebp, 1					; 1
;			nop
			fmulp	st3, st0			    ; 1 [0]ifrac2 [1]samp2+1 [2]interp1 [3]samp1 [4]samp2
			fmul	dword [mix_1over2gig]	; 1 [0]ffrac2 [1]samp2+1 [2]interp1 [3]samp1 [4]samp2
			fxch	st1					    ;   [0]samp2+1 [1]ffrac2 [2]interp1 [3]samp1 [4]samp2
			fsub	st0, st4			    ; 1 [0]delta2 [1]ffrac2 [2]interp1 [3]samp1 [4]samp2
			add		ebp, ebx				; 1
;			nop
			adc		esi, ecx				; 1
;			nop
			fmulp	st1, st0			    ; 1 [0]interp2 [1]interp1 [2]samp1 [3]samp2
			fxch	st1					    ;   [0]interp1 [1]interp2 [2]samp1 [3]samp2
			faddp	st2, st0			    ; 1 [0]interp2 [1]newsamp1 [2]samp2
;			fnop							; 1 fadd stall
;			fnop							; 1 fadd stall
			fld		st1					    ; 1 [0]newsamp1 [1]interp2 [2]newsamp1 [3]samp2
			fmul	dword [mix_leftvol]		; 1 [0]newsampL1 [1]interp2 [2]newsamp1 [3]samp2
			fxch	st1					    ;   [0]interp2 [1]newsampL1 [2]newsamp1 [3]samp2
			faddp	st3, st0			    ; 1 [0]newsampL1 [1]newsamp1 [2]newsamp2
			fxch	st1					    ; 1 [0]newsamp1 [1]newsampL1 [2]newsamp2
			jmp		MixLoopUnroll16CoilEntry; 1

			ALIGN 16

		MixLoopUnroll16:
			shr		ebp, 1					; 1
;			nop
			mov		[mix_temp1], ebp		; 1
;			nop
			fild	word [esi+esi+2]	    ; 1 [0]samp1+1 [1]finalR2 [2]finalR1 [3]finalL2
			fild	word [esi+esi]		    ; 1 [0]samp1 [1]samp1+1 [2]finalR2 [3]finalR1 [4]finalL2
			fild	dword [mix_temp1]		; 1 [0]ifrac1 [1]samp1 [2]samp1+1 [3]finalR2 [4]finalR1 [5]finalL2
			add		ebp, ebp				; 1
;			nop
			add		ebp, ebx				; 1
;			nop
			adc		esi, ecx				; 1
			add		edi, 16					;
			fmul	dword [mix_1over2gig]	; 1 [0]ffrac1 [1]samp1 [2]samp1+1 [3]finalR2 [4]finalR1 [5]finalL2
			shr		ebp, 1					; 1
;			nop
			mov		[mix_temp1], ebp		; 1
;			nop
			fild	dword [mix_temp1]		; 1 [0]ifrac2 [1]ffrac1 [2]samp1 [3]samp1+1 [4]finalR2 [5]finalR1 [6]finalL2
			fild	word [esi+esi+2]	    ; 1 [0]samp2+1 [1]ifrac2 [2]ffrac1 [3]samp1 [4]samp1+1 [5]finalR2 [6]finalR1 [7]finalL2
			fxch	st4					    ;   [0]samp1+1 [1]ifrac2 [2]ffrac1 [3]samp1 [4]samp2+1 [5]finalR2 [6]finalR1 [7]finalL2
			fsub	st0, st3			    ; 1 [0]delta1 [1]ifrac2 [2]ffrac1 [3]samp1 [4]samp2+1 [5]finalR2 [6]finalR1 [7]finalL2
;			fnop							; 1 fsub stall
			shl		ebp, 1					; 1
;			nop
			fmulp	st2, st0			    ; 1 [0]ifrac2 [1]interp1 [2]samp1 [3]samp2+1 [4]finalR2 [5]finalR1 [6]finalL2
			fild	word [esi+esi]		    ; 1 [0]samp2 [1]ifrac2 [2]interp1 [3]samp1 [4]samp2+1 [5]finalR2 [6]finalR1 [7]finalL2
			fxch	st1					    ;   [0]ifrac2 [1]samp2 [2]interp1 [3]samp1 [4]samp2+1 [5]finalR2 [6]finalR1 [7]finalL2
			fmul	dword [mix_1over2gig]	; 1 [0]ffrac2 [1]samp2 [2]interp1 [3]samp1 [4]samp2+1 [5]finalR2 [6]finalR1 [7]finalL2
			fxch	st4					    ;   [0]samp2+1 [1]samp2 [2]interp1 [3]samp1 [4]ffrac2 [5]finalR2 [6]finalR1 [7]finalL2
			fsub	st0, st1			    ; 1 [0]delta2 [1]samp2 [2]interp1 [3]samp1 [4]ffrac2 [5]finalR2 [6]finalR1 [7]finalL2
			add		ebp, ebx				; 1
;			nop
			adc		esi, ecx				; 1
;			nop
			fmulp	st4, st0			    ; 1 [0]samp2 [1]interp1 [2]samp1 [3]interp2 [4]finalR2 [5]finalR1 [6]finalL2
			fxch	st2					    ;   [0]samp1 [1]interp1 [2]samp2 [3]interp2 [4]finalR2 [5]finalR1 [6]finalL2
			faddp	st1, st0			    ; 1 [0]newsamp1 [1]samp2 [2]interp2 [3]finalR2 [4]finalR1 [5]finalL2
			fxch	st4					    ;	 [0]finalR1 [1]samp2 [2]interp2 [3]finalR2 [4]newsamp1 [5]finalL2
			fstp	dword [edi-28]		    ; 2 [0]samp2 [1]interp2 [2]finalR2 [3]newsamp1 [4]finalL2
			fxch	st4					    ; 1 [0]finalL2 [1]interp2 [2]finalR2 [3]newsamp1 [4]samp2
			fstp	dword [edi-24]		    ; 2 [0]interp2 [1]finalR2 [2]newsamp1 [3]samp2
			fld		st2					    ; 1 [0]newsamp1 [1]interp2 [2]finalR2 [3]newsamp1 [4]samp2
			fmul	dword [mix_leftvol]		; 1 [0]newsampL1 [1]interp2 [2]finalR2 [3]newsamp1 [4]samp2
			fxch	st1					    ;   [0]interp2 [1]newsampL1 [2]finalR2 [3]newsamp1 [4]samp2
			faddp	st4, st0			    ; 1 [0]newsampL1 [1]finalR2 [2]newsamp1 [3]newsamp2
			fxch	st1					    ;   [0]finalR2 [1]newsampL1 [2]newsamp1 [3]newsamp2
			fstp	dword [edi-20]		    ; 2 [0]newsampL1 [1]newsamp1 [2]newsamp2
			fxch	st1					    ; 1 [0]newsamp1 [1]newsampL1 [2]newsamp2

		MixLoopUnroll16CoilEntry:			;   [0]newsamp1 [1]newsampL1 [2]newsamp2

			fmul	dword [mix_rightvol]	; 1 [0]newsampR1 [1]newsampL1 [2]newsamp2
			fxch	st2					    ;   [0]newsamp2 [1]newsampL1 [2]newsampR1
			fld		st0					    ; 1 [0]newsamp2 [1]newsamp2 [2]newsampL1 [3]newsampR1
			fmul	dword [mix_leftvol]		; 1 [0]newsampL2 [1]newsamp2 [2]newsampL1 [3]newsampR1
			fxch	st1					    ;   [0]newsamp2 [1]newsampL2 [2]newsampL1 [3]newsampR1
;			fnop							; 1 delay on mul unit
			fmul	dword [mix_rightvol]	; 1 [0]newsampR2 [1]newsampL2 [2]newsampL1 [3]newsampR1
			fxch	st2					    ;   [0]newsampL1 [1]newsampL2 [2]newsampR2 [3]newsampR1
			fadd	dword [edi-16]		    ; 1 [0]finalL1 [1]newsampL2 [2]newsampR2 [3]newsampR1
			fxch	st3					    ;   [0]newsampR1 [1]newsampL2 [2]newsampR2 [3]finalL1
			fadd	dword [edi-12]		    ; 1 [0]finalR1 [1]newsampL2 [2]newsampR2 [3]finalL1
			fxch	st1					    ;   [0]newsampL2 [1]finalR1 [2]newsampR2 [3]finalL1
			fadd	dword [edi-8]		    ; 1 [0]finalL2 [1]finalR1 [2]newsampR2 [3]finalL1
			fxch	st3					    ;   [0]finalL1 [1]finalR1 [2]newsampR2 [3]finalL2
;			fnop							; 1 delay on store?
			fstp	dword [edi-16]		    ; 2 [0]finalR1 [1]newsampR2 [2]finalL2
			fxch	st1					    ; 1 [0]newsampR2 [1]finalR1 [2]finalL2
			fadd	dword [edi-4]		    ; 1 [0]finalR2 [1]finalR1 [2]finalL2

			dec		edx						; 1
			jnz		MixLoopUnroll16			;

			fxch	st1					    ; 1 [0]finalR1 [1]finalR2 [2]finalL2
			fstp	dword [edi-12]		    ; 2 [0]finalR2 [1]finalL2
			fxch	st1					    ; 1 [0]finalL2 [1]finalR2
			fstp	dword [edi-8]		    ; 2 [0]finalR2
			fstp	dword [edi-4]		    ; 2

			;= MIX 16BIT, ROLLED ==================================================================
		MixLoopStart16:
			mov		edx, [mix_count]

	%ifndef ROLLED
			and		edx, 1
	%endif

	%ifdef VOLUMERAMPING
		MixLoopStart16_2:
	%endif
			or		edx, edx					; if count == 0 dont enter the mix loop
			jz		MixLoopEnd16

	%ifdef VOLUMERAMPING
			fld		dword [mix_rampspeedleft]		; [0]rampspeedL
			fld		dword [mix_rampspeedright]		; [0]rampspeedR [1]rampspeedL
			fld		dword [mix_rampleftvol]			; [0]lvol [1]rampspeedR [2]rampspeedL
			fld		dword [mix_ramprightvol]		; [0]rvol [1]lvol [2]rampspeedR [3]rampspeedL
	%else
			fldz						            ; [0]rampspeedL
			fldz						            ; [0]rampspeedR [1]rampspeedL
			fld		dword [mix_leftvol]			    ; [0]lvol [1]rampspeedR [2]rampspeedL
			fld		dword [mix_rightvol]		    ; [0]rvol [1]lvol [2]rampspeedR [3]rampspeedL
	%endif
			jmp		MixLoop16

			ALIGN 16

		MixLoop16:
			shr		ebp, 1					; 1 make 31bit coz fpu only loads signed values
			add		edi, 8					;
			fild	word [esi+esi+2]	    ; 1 [0]samp1+1 [1]rvol [2]lvol [3]rampspeedR [4]rampspeedL
			mov		[mix_temp1], ebp		; 1
			fild	word [esi+esi]		    ; 1 [0]samp1 [2]samp1+1 [3]rvol [4]lvol [5]rampspeedR [6]rampspeedL
			fild	dword [mix_temp1]		; 1 [0]ifrac [1]samp1 [2]samp1+1 [3]rvol [4]lvol [5]rampspeedR [6]rampspeedL
			shl		ebp, 1					; 1 restore mixpos low
			add		ebp, ebx				;   add speed low to mixpos low
			adc		esi, ecx				; 1 add upper portion of speed plus carry
;			nop
			fmul	dword [mix_1over2gig]	; 1 [0]ifrac [1]samp1 [2]samp1+1 [3]rvol [4]lvol [5]rampspeedR [6]rampspeedL
			fxch	st2					    ;   [0]samp1+1 [1]samp1 [2]ffrac [3]rvol [4]lvol [5]rampspeedR [6]rampspeedL
			fsub	st0, st1			    ; 1 [0]delta1 [1]samp1 [2]ffrac [3]rvol [4]lvol [5]rampspeedR [6]rampspeedL
;			fnop							; 1
;			fnop							; 1
;			fnop							; 1
			fmulp	st2, st0			    ; 1 [0]sample [1]interp [2]rvol [3]lvol [4]rampspeedR [5]rampspeedL
;			fnop							; 1
;			fnop							; 1
;			fnop							; 1
;			fnop							; 1
			faddp	st1, st0			    ; 1 [0]newsamp [1]rvol [2]lvol [3]rampspeedR [4]rampspeedL
;			fnop							; 1
;			fnop							; 1
			fld		st0					    ; 1 [0]newsamp [1]newsamp [2]rvol [3]lvol [4]rampspeedR [5]rampspeedL
			fmul	st0, st3			    ; 1 [0]newsampL [1]newsamp [2]rvol [3]lvol [4]rampspeedR [5]rampspeedL
			fxch	st3					    ;   [0]lvol [1]newsamp [2]rvol [3]newsampL [4]rampspeedR [5]rampspeedL
			fadd	st0, st5			    ; 1 [0]lvol [1]newsamp [2]rvol [3]newsampL [4]rampspeedR [5]rampspeedL
			fxch	st1					    ;   [0]newsamp [1]lvol [2]rvol [3]newsampL [4]rampspeedR [5]rampspeedL
			fmul	st0, st2			    ; 1 [0]newsampR [1]lvol [2]rvol [3]newsampL [4]rampspeedR [5]rampspeedL
			fxch	st2					    ;   [0]rvol [1]lvol [2]newsampR [3]newsampL [4]rampspeedR [5]rampspeedL
			fadd	st0, st4			    ; 1 [0]rvol [1]lvol [2]newsampR [3]newsampL [4]rampspeedR [5]rampspeedL
			fxch	st3					    ;   [0]newsampL [1]lvol [2]newsampR [3]rvol [4]rampspeedR [5]rampspeedL
;			fnop							; 1
;			fnop							; 1
			fadd	dword [edi-8]		    ; 1 [0]finalL [1]lvol [2]newsampR [3]rvol [4]rampspeedR [5]rampspeedL
			fxch	st2					    ;   [0]newsampR [1]lvol [2]finalL [3]rvol [4]rampspeedR [5]rampspeedL
			fadd	dword [edi-4]		    ; 1 [0]finalR [1]lvol [2]finalL [3]rvol [4]rampspeedR [5]rampspeedL
			fxch	st2					    ;   [0]finalL [1]lvol [2]finalR [3]rvol [4]rampspeedR [5]rampspeedL
;			fnop							; 1
			fstp	dword [edi-8]		    ; 1 [0]lvol [1]finalR [2]rvol [3]rampspeedR [4]rampspeedL
			fxch	st1					    ;   [0]finalR [1]lvol [2]rvol [3]rampspeedR [4]rampspeedL
			fstp	dword [edi-4]		    ; 3 [0]lvol [1]rvol [2]rampspeedR [3]rampspeedL
			fxch	st1					    ;   [0]rvol [1]lvol [2]rampspeedR [3]rampspeedL

			dec		edx						; 1
			jnz		MixLoop16				;

			fxch	st2					            ; [0]rampspeedR [1]lvol [2]rvol [3]rampspeedL
			fstp	dword [mix_rampspeedright]		; [0]lvol [1]rvol [2]rampspeedL
			fxch	st2					            ; [0]rampspeedL [1]rvol [2]lvol
			fstp	dword [mix_rampspeedleft]		; [0]rvol [1]lvol

			fmul	dword [mix_255]					; [0]rvol*255 [1]lvol
			fmul	dword [mix_256]					; [0]rvol*255*256 [1]lvol
			fxch	st1					            ; [0]lvol [1]rvol*255*256
			fmul	dword [mix_255]					; [0]lvol*255 [1]rvol*255*256
			fmul	dword [mix_256]					; [0]lvol*255*256 [1]rvol*255*256

			xor		eax, eax
			fistp	dword [mix_rampleftvol]			; [0]rvol*255*256
			fistp	dword [mix_ramprightvol]		;

		MixLoopEnd16:
			mov		eax, [mix_samplebuff]
			shr		eax, 1
			sub		esi, eax

%ifdef VOLUMERAMPING
			;=============================================================================================
			; DID A VOLUME RAMP JUST HAPPEN
			;=============================================================================================
			cmp		dword [mix_rampcount], 0
			je		DoOutputbuffEnd		; no, no ramp

			mov		ebx, [mix_sptr]		; load ebx with sample pointer
			mov		ecx, [mix_cptr]		; load ecx with channel pointer

			mov		eax, [mix_rampleftvol]
			mov		[ecx + Channel.ramp_leftvolume], eax
			mov		eax, [mix_ramprightvol]
			mov		[ecx + Channel.ramp_rightvolume], eax

			mov		eax, [mix_count]
			mov		edx, [mix_rampcount]

			sub		edx, eax

			mov		dword [mix_rampspeedleft], 0		; clear out volume ramp
			mov		dword [mix_rampspeedright], 0		; clear out volume ramp
			mov		[mix_rampcount], edx
			mov		[ecx + Channel.ramp_count], edx

			; if rampcount now = 0, a ramp has FINISHED, so finish the rest of the mix
			cmp		edx, 0
			jne		DoOutputbuffEnd

			; clear out the ramp speeds
			mov		dword [ecx + Channel.ramp_leftspeed], 0
			mov		dword [ecx + Channel.ramp_rightspeed], 0

			; clamp the 2 volumes together in case the speed wasnt accurate enough!
			mov		edx, [ecx + Channel.leftvolume]
			shl		edx, 8
			mov		[ecx + Channel.ramp_leftvolume], edx
			mov		edx, [ecx + Channel.rightvolume]
			shl		edx, 8
			mov		[ecx + Channel.ramp_rightvolume], edx

			; is it 0 because ramp ended only? or both ended together??
			; if sample ended together with ramp.. problems .. loop isnt handled

			cmp		[mix_count_old], eax		; ramp and output mode ended together
			je		DoOutputbuffEnd

			; start again and continue rest of mix
			mov		[ecx + Channel.mixpos], esi
			mov		[ecx + Channel.mixposlo], ebp

			mov		eax, [mix_mixbuffend]	; find out how many OUTPUT samples left to mix
			sub		eax, edi
			shr		eax, 3				; eax now holds # of samples left, go recalculate mix_count!!!
			mov		[mix_mixbuffptr], edi	; update the new mixbuffer pointer

			cmp		eax, 0				; dont start again if nothing left
			jne		CalculateLoopCount

		DoOutputbuffEnd:
%endif
			cmp		dword [mix_endflag], FSOUND_OUTPUTBUFF_END
			je		FinishUpChannel

			;=============================================================================================
			; SWITCH ON LOOP MODE TYPE
			;=============================================================================================
			mov		ebx, [mix_sptr]				; load ebx with sample pointer
			mov		ecx, [mix_cptr]				; load ecx with sample pointer

			mov		dl,	[ebx + Sample.loopmode]

			; check for normal loop
			test	dl, FSOUND_LOOP_NORMAL
			jz		CheckBidiLoop

			mov		eax, [ebx + Sample.loopstart]
			add		eax, [ebx + Sample.looplen]
		rewindsample:
			sub		esi, [ebx + Sample.looplen]
			cmp		esi, eax
			jae		rewindsample

			mov		[ecx + Channel.mixpos], esi
			mov		[ecx + Channel.mixposlo], ebp
			mov		eax, [mix_mixbuffend]			; find out how many samples left to mix for the output buffer
			sub		eax, edi
			shr		eax, 3						; eax now holds # of samples left, go recalculate mix_count!!!
			mov		[mix_mixbuffptr], edi			; update the new mixbuffer pointer

			cmp		eax, 0
			je		FinishUpChannel

			jmp		CalculateLoopCount

		CheckBidiLoop:
			test	dl, FSOUND_LOOP_BIDI
			jz		NoLoop
			cmp		dword [ecx + Channel.speeddir], FSOUND_MIXDIR_FORWARDS
			je		BidiForward

		BidiBackwards:
			mov		eax, [ebx + Sample.loopstart]
			dec		eax
	;		mov		edx, 0ffffff00h
			mov		edx, 0ffffffffh
			sub		edx, ebp
			sbb		eax, esi

			mov		esi, eax
			mov		ebp, edx					; esi:ebp = loopstart - mixpos
			mov		eax, [ebx + Sample.loopstart]
			mov		edx, 0h
			add		ebp, edx
			adc		esi, eax					; esi:ebp += loopstart

			mov		dword [ecx + Channel.speeddir], FSOUND_MIXDIR_FORWARDS

			mov		eax, [ebx + Sample.loopstart]
			add		eax, [ebx + Sample.looplen]
			cmp		esi, eax
			jge		BidiForward

			jmp		BidiFinish
		BidiForward:

			mov		eax, [ebx + Sample.loopstart]
			add		eax, [ebx + Sample.looplen]
			mov		edx, 0h
			sub		edx, ebp
			sbb		eax, esi
			mov		esi, eax
			mov		ebp, edx					; esi:ebp = loopstart+looplen - mixpos

			mov		eax, [ebx + Sample.loopstart]
			add		eax, [ebx + Sample.looplen]
			dec		eax
	;		mov		edx, 0ffffff00h
			mov		edx, 0ffffffffh
			add		ebp, edx
			adc		esi, eax

			mov		dword [ecx + Channel.speeddir], FSOUND_MIXDIR_BACKWARDS

			cmp		esi, [ebx + Sample.loopstart]
			jl		BidiBackwards

		BidiFinish:

			mov		[ecx + Channel.mixpos], esi
			mov		[ecx + Channel.mixposlo], ebp

			mov		eax, [mix_mixbuffend]			; find out how many samples left to mix for the output buffer
			sub		eax, edi
			shr		eax, 3						; eax now holds # of samples left, go recalculate mix_count!!!
			mov		[mix_mixbuffptr], edi			; update the new mixbuffer pointer

			cmp		eax, 0
			je		FinishUpChannel

			jmp		CalculateLoopCount

		NoLoop:
			xor		ebp, ebp
			xor		esi, esi
			mov		[ecx + Channel.sptr], esi		; clear the sample pointer out

			;= LEAVE INNER LOOP
		FinishUpChannel:
			mov		ecx, [mix_cptr]

			mov		[ecx + Channel.mixposlo], ebp
			mov		[ecx + Channel.mixpos], esi			; reset mixpos based on esi for next time around

		;===================================================================================================
		; EXIT
		;===================================================================================================
		MixExit:
            dec dword [count]
            jz MixFinished
            add dword [cptr], Channel.size
            jmp MixChannelStart

		MixFinished:
		    popad
			pop		ebp						;= RESTORE EBP
			ret
