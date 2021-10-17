.586P	

	
descr	struc
	lim		dw	0
	base_l	dw	0
	base_m	db	0
	attr_1	db	0
	attr_2	db	0
	base_h	db	0
descr	ends

idescr struc
    offs_l  dw 0
    sel     dw 0
    cntr    db 0 
    attr    db 0
    offs_h  dw 0
idescr ends

stack32 segment  para stack 'STACK'
    stack_start db  100h dup(?)
    stack_size = $-stack_start
stack32 ends

data16 segment para 'data' use16
    gdt_null  descr <>
	gdt_code16 descr <code16_size-1,0,0,98h,0,0>
    gdt_data16 descr <data16_size-1,0,0,92h,0,0>
    gdt_code32 descr <code32_size-1,0,0,98h,40h,0>
	gdt_data32 descr <0FFFFh,0,0,92h,0CFh,0>
    gdt_stack32 descr <stack_size-1,0,0,92h,40h,0>
	gdt_screen16 descr <3999,8000h,0Bh,92h,0,0> 
	
	gdt_size=$-gdt_null
	
	pdescr	df	0
	
    code16s=8
    data16s=16
    code32s=24
    data32s=32
    stack32s=40
    video16s=48
	
    idt label byte
	
    idescr_0_12 idescr 13 dup (<0,code32s,0,8Fh,0>) 
    
    idescr_13 idescr <0,code32s,0,8Fh,0>
    
    idescr_14_31 idescr 18 dup (<0,code32s,0,8Fh,0>)

	;1000 1110b <=> 8Eh шлюз прерывания
    int08 idescr <0,code32s,0,10001110b,0> 
    int09 idescr <0,code32s,0,10001110b,0>

    idt_size=$-idt

    ipdescr df 0

    ipdescr16 dw 3FFh, 0, 0 

    mask_master db 0        
    mask_slave  db 0        
	
	asciimap_high db 0
	db 0 ;1 - Esc
	db '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '='
	db 219 ;14 - BackSpace
	db 0 ;15 - TAB
    db 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}'
	db 3 ;28 - Enter
	db 0 ;29 - LeftCtrl
    db 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~'
	db 0 ;42 - LeftShift
	db 92 ;43 - '/'
	db 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?' ;53 символа
	
	shift_reg_flag db 0
	caps_lock_flag db 0
	
	asciimap_low db 0
	db 0 ;1 - Esc
	db '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '='
	db 219 ;14 - BackSpace
	db 0 ;15 - TAB
    db 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']'
	db 3 ;28 - Enter
	db 0 ;29 - LeftCtrl
    db 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', 39, '`'
	db 0 ;42 - LeftShift
	db 92 ;43 - '/'
	db 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/' ;53 символа
	;db 0 ;54 - RightShift
	;db 14 ;55 - PrintScreen
	;db 1 ;56 - Alt*
	;db ' ' ;57 - Пробел
	;db 0 ;58 - CapsLock

    flag_enter_pr db 0
    cnt_time      db 0            

    syml_pos      dd 2 * 80 * 5
	cursor_pos=2 * 80 * 5 ; 80 * 2 - длина строки. (расширение 80x25 (возможно))
    cursor_symb=219
    param_int_8 db 00Fh ; Цвет курсора.
	
	
	mem_pos=0
    ; позиция на экране значения кол-ва доступной памяти
    mem_value_pos=14 + 16 ; 14: пропускаем нашу строку (это ее длина), 
    ; 16: FFFF FFFF - max возможное значение, кот-ое мы можем вывести, 
    ; Длина = 8, умножаем на 2, т.к. там еще атрибут учитывается.
    mb_pos=30 + 2
    param=1Eh

	rm_msg      db 27, '[30;42mNow in Real Mode. ', 27, '[0m$', '$'
    pm_msg_wait db 27, '[30;42mPress any button to enter protected mode!', 27, '[0m$'
    pm_msg_out  db 27, '[30;42mNow in Real Mode again! ', 27, '[0m$'
    pm_mem_count db 'Memory: '
	
	data16_size=$-gdt_null
data16 ends


code32 segment para public 'code' use32
    assume cs:code32, ds:data16, ss:stack32
	
pm_start:
    mov ax, data16s
    mov ds, ax
    mov ax, video16s
    mov es, ax
    mov ax, stack32s
    mov ss, ax
    mov eax, stack_size
    mov esp, eax

    sti ; Резрешаем (аппаратные) прерывания
	
	; Вывод сообщения "Memory"
    mov di, mem_pos
    mov ah, param
    xor esi, esi
    xor ecx, ecx
    mov cx, 8 ; Длина строки
    print_memory_msg:
        mov al, pm_mem_count[esi]
        stosw ; al (символ) с параметром (ah) перемещается в область памяти es:di
        inc esi
    loop print_memory_msg

    ; Считаем и выводим кол-во физической памяти, выделенной dosbox'у.
    call count_memory_proc
	    
    ; Цикл, пока не будет введен Enter
    proccess:
        test flag_enter_pr, 1 ; если flag = 1, то выход
    jz  proccess
	    
    cli ; Запрет аппаратных маскируемых прерывания прерываний.
	
    ; Выход из защищенного режима
    db  0EAh ; jmp
    dd  offset return_rm ; offset
    dw  code16s ; selector
	
	; Заглушка для исключений.
    except_1 proc
        iret
    except_1 endp

    ; Заглушка для 13 исключения.
    ; Нужно снять со стека код ошибки.
    except_13 proc uses eax
        pop eax
        iret
    except_13 endp
	
	
	new_int08 proc uses eax edi
		cmp syml_pos,0
        mov edi, cursor_pos
		je use_cursor_pos ; поместим в edi позицию для вывода
		mov edi, syml_pos
	use_cursor_pos:

        mov ah, param_int_8 ; В ah помещаем цвет текста.
        ror ah, 1           ; Сдвигаем циклически вправо параметр (он примет какое-то новое значение) 
        ; Т.е. он будет меняться вот так:
        ; 0000 1111 -> 0001 1110 -> 0011 1100 и т.д. циклически.
        mov param_int_8, ah
        mov al, cursor_symb ; Символ, который мы хотим вывести (в моем случае просто квадрат).
        stosw ; al (символ) с параметром (ah) перемещается в область памяти es:di


        ; используется только в аппаратных прерываниях для корректного завершения
        ; (разрешаем обработку прерываний с меньшим приоритетом)!!
        mov al, 20h
        out 20h, al

        iretd ; double - 32 битный iret
    new_int08 endp
	
	
	; uses - сохраняет контекст (push + pop)
    new_int09 proc uses eax ebx edx
        ;Порт 60h при чтении содержит скан-код последней нажатой клавиши.
        in  al, 60h ; считываем порт клавы
		
		;Пока считаем память - печатать нельзя
		mov ebx, syml_pos 		
		cmp ebx,0
		je exit
		
		; игнорим Ctrl
		cmp al, 29
		je exit
		
		;Debug output
		;MOV DL,AL
		;MOV DH,param
		;MOV ES:[80],DX		
		
	; Проверяем Enter
        cmp al, 28
        jne check_caps_on         
        or flag_enter_pr, 1
        jmp exit
	
	; проверяем CapsLock вкл
	check_caps_on:
		cmp al,58
		jne check_caps_off
		or caps_lock_flag,1
		jmp exit
	
	; проверяем CapsLock Выкл
	check_caps_off:
		cmp al,186
		jne check_shift_pressed
		and caps_lock_flag,0
		jmp exit
		
	; проверяем LeftShift pressed
	check_shift_pressed:
		cmp al,42
		jne check_shift_unpressed
		or shift_reg_flag,1
		jmp exit

	; проверяем LeftShift unpressed
	check_shift_unpressed:
		cmp al,170
		jne check_BackSpace
		and shift_reg_flag,0
		jmp exit
		
	; проверяем BackSpace
	check_BackSpace:
		cmp al, 0Eh
        jne check_probel
        mov ebx, syml_pos
		mov es:[ebx], ' '
		sub ebx, 2
        mov syml_pos, ebx
		jmp exit
	
	; проверяем пробел
	check_probel:
		cmp al,57
		jne check_tab
		mov dl, ' '
		jmp print_dx
		
	; проверяем Tab
	check_tab:
		cmp al,15
		jne set_value
        mov ebx, syml_pos
		mov dword ptr es:[ebx], 1E201E20h
		mov dword ptr es:[ebx+4], 1E201E20h
		add ebx, 8
        mov syml_pos, ebx		
		jmp exit
	
    set_value:
        ; это условие проверяет, отпущена ли была клавиша (то есть если al > 80h, то клавиша была отпущена)
        cmp al, 80h
        ja exit

        xor ah, ah
		
		cmp ax,2
		jb exit
		cmp ax,53
		ja exit

        xor ebx, ebx
        mov bx, ax
		
        mov dl, asciimap_low[ebx]
		
		mov dh, caps_lock_flag
		xor dh, shift_reg_flag
		cmp dh, 0
		je print_dx
		
		mov dl, asciimap_high[ebx] 	
	print_dx:
        mov dh, param	
        mov ebx, syml_pos 		
		
        mov es:[ebx], dx
        add ebx, 2   
        mov syml_pos, ebx

    exit: 
        ; используется только в аппаратных прерываниях для корректного завершения (разрешаем обработку прерываний с меньшим приоритетом)!!
        mov al, 20h 
        out 20h, al

        iretd
    new_int09 endp
	
	
	; USES - список регистров, значения которых изменяет процедура. Ассемблер по-
    ; мещает в начало процедуры набор команд PUSH, а перед командой RET - набор
    ; команд POP, так что значения перечисленных регистров будут восстановлены
    ; Если ds не сохранить, то вернувшись обратно ds будет содержать селектор data32s.
    count_memory_proc proc uses ds eax ebx
        mov ax, data32s ; Селектор, который указывает на дескриптор, описывающий сегмент 4 Гб.
        mov ds, ax ; На данном этапе в сегментный регистр помещается селектор data32s
        ;  И в этот же момент в теневой регистр помещается дескриптор gdt_data32

        ; Перепрыгиваем первый мегабайт 2^20.
        ; Т.к в первом мегобайте располагается наша программа
        ; И в какой-то момент мы можем затереть команду и все рухнет.
        ; Счетчик (кол-во памяти).
        mov ebx,  100001h ; (16^5 + 1 == (2^4)^5 + 1 == 2^20 + 1) 2^20 + 1 байт. (можно не делать +1, первый мегабайт начинается с байта с индексом 2^20)
        mov dl,   0AEh ; Некоторое значение, с помощью которого мы будем проверять запись.

        ; Это оставшееся FFEF FFFE + 10 0001 = 10000 0000 ==  (2^4)^8 = 2^32  = 4 Гб
        mov ecx, 0FFEFFFFEh

        count_memory:
            ; Сохраняем байт в dh.
            mov dh, ds:[ebx] ; ds:[ebx] - линейный адрес вида: 0 + ebx (ebx пробегает)
			; Записываем по этому адресу сигнатуру.
            mov ds:[ebx], dl   
            ; Сравниваем записанную сигнатуру с сигнатурой в программе.
            cmp ds:[ebx], dl    
        
            ; Если не равны, то это уже не наша память. Выводим посчитанное кол-во.
            jne print_memory_counter
        
            mov ds:[ebx], dh    ; Обратно запиываем считанное значени.
            inc ebx             ; Увеличиваем счетчик.
        loop count_memory
    print_memory_counter:
        mov eax, ebx 
        xor edx, edx

        ; Мы считали по байту. Переводим в мегабайты.
        ; Делим на 2^20 (кол-во байт в мегабайте).
        mov ebx, 100000h ; 16^5 = (2^4)^5 = 2^20
        div ebx ; делим eax / ebx -> eax содержит кол-во МЕГАБАЙТ.

        mov ebx, mem_value_pos
        ; функция, которая печатает eax (в котором лежит найденное кол-во мегабайт)
        call print_count_memory
		; Печать символа h
        mov ah, param
        mov ebx, mb_pos
        mov al, 'h'
        mov es:[ebx], ax
		; Печать пробела
        mov ah, param
        mov ebx, mb_pos + 2
        mov al, ' '
        mov es:[ebx], ax
        ; Печать надписи Mb (мегабайты)
        mov ah, param
        mov ebx, mb_pos + 4
        mov al, 'M'
        mov es:[ebx], ax

        mov ebx, mb_pos + 6
        mov al, 'b'
        mov es:[ebx], ax
        ret

    count_memory_proc endp

    print_count_memory proc uses ecx ebx edx
        ; В eax лежит кол-во мегабайт.
        ; В ebx лежит mem_value_pos.
        mov ecx, 8
        mov dh, param

        print_symbol:
            mov dl, al
            ; Получаем "младшую часть dl"
            and dl, 0Fh ; AND с 0000 1111 --> остаются последние 4 бита, то есть 16ричная цифра

            ; Сравниваем с 10
            cmp dl, 10
            ; Если dl меньше 10, то выводим просто эту цифру.
            jl to_ten ; До десяти.
            ; Если больше 10, то вычитаем из dl 10
            ; Тем самым получая все, что больше 10
            ; Т.е. 0 или 1 или ... или 5
            sub dl, 10
            ; Добавляем к dl 'A', то есть то, что больше 10
            ; И получаем  собственно нужную нам букву.
            add dl, 'A'
            jmp after_ten

        
        to_ten:
            add dl, '0'  ; Превращаем в строковое представление число.
        after_ten:
            ; Помещаем в видеобуфер dx 
            mov es:[ebx], dx 
            ; Циклически сдвигаем вправо число на 4, 
            ; Тем самым на след. операции будем работать со след. цифрой.
            ror eax, 4 ; убираем последнюю 16ричную цифру eax
            ; 2 - т.к. байт атрибутов и байт самого символа.
            sub ebx, 2 ; переходим к левой ячейки видеопамяти       
        loop print_symbol

        ret
    print_count_memory endp

    code32_size = $-pm_start
code32 ends


code16 	segment para public 'CODE' use16
		assume cs:code16, ds:data16, ss: stack32
		
; Перенес на новую строку.
NewLine: 
    xor dx, dx
    mov ah, 2  ; Номер команды, для вывода символа. 
    mov dl, 13 ; Возврат коретки.
    int 21h    
    mov dl, 10 ; Перенос на новую строку.
    int 21h
    ret

; Очистка экрана
ClearScreen:
    mov ax, 3
    int 10h
    ret
		
; Начало программы.
main:
    ; Инициализируем DS сегментом данных.
	mov	AX,data16
	mov	DS,AX
	
	; Вывдим сообщение, о том, что мы в реальном режиме.
	mov ah, 09h
	; lea == mov dx, offset ds:rm_msg
	lea dx, rm_msg
	int 21h
	call NewLine

	; Вывод сообщения, что мы ожидаем нажатие клавиши. 
	mov ah, 09h
	lea dx, pm_msg_wait
	int 21h
	call NewLine

	; Ожидание нажатия кнопки
	mov ah, 10h
	int 16h
	
	call ClearScreen
	
	xor eax, eax
	
	; Просто записываем линейные адреса в дескрипторы сегментов
	; Линейные (32-битовые) адреса определяются путем умножения значений
	; Сегментных адресов на 16.
	mov ax, code16
    shl eax, 4 ; Получаем линейный адрес                    
    mov word ptr gdt_code16.base_l, ax  
    shr eax, 16                       
    mov byte ptr gdt_code16.base_m, al  
    mov byte ptr gdt_code16.base_h, ah  
	
    ; просто записываем линейные адреса в дескрипторы сегментов
    mov ax, code32
    shl eax, 4                        
    mov word ptr gdt_code32.base_l, ax  
    shr eax, 16                       
    mov byte ptr gdt_code32.base_m, al  
    mov byte ptr gdt_code32.base_h, ah  

    ; просто записываем линейные адреса в дескрипторы сегментов
    mov ax, data16
    shl eax, 4                        
    mov word ptr gdt_data16.base_l, ax  
    shr eax, 16                       
    mov byte ptr gdt_data16.base_m, al  
    mov byte ptr gdt_data16.base_h, ah  

    ; просто записываем линейные адреса в дескрипторы сегментов
    mov ax, stack32
    shl eax, 4                        
    mov word ptr gdt_stack32.base_l, ax  
    shr eax, 16                       
    mov byte ptr gdt_stack32.base_m, al  
    mov byte ptr gdt_stack32.base_h, ah
	
	; получаем адрес сегмента, где лежит глобальная таблица дескрипторов
    mov ax, data16
    shl eax, 4
    ; Прибавляем смещение этой таблицы в этом сегменте к начальному адресу сегменту 
    ; И получаем ----> Линейный адрес таблицы GDT.
    add eax, offset gdt_null
	
	; размер GDTR - 32 бита (4 байта)
    ; LGDT (Load GDT) - загружает в регистр процессора GDTR (GDT Register)  (лежит лин. адр этой табл и размер)
    ; (LGDT относится к типу привилегированных команд.)
	mov	dword ptr pdescr+2,eax;
	mov	word ptr pdescr,gdt_size-1	
	lgdt pdescr
	
	mov ax, code32
    mov es, ax
		
	; Добавили IDTR
	; Заносим в дескрипторы прерываний (шлюзы) смешение обработчиков прерываний.
    lea eax, es:except_1 ; Тоже самое, что и `mov eax, offset es:except_1`
    mov idescr_0_12.offs_l, ax
    shr eax, 16
    mov idescr_0_12.offs_h, ax

    lea eax, es:except_13
    mov idescr_13.offs_l, ax 
    shr eax, 16             
    mov idescr_13.offs_h, ax 

    lea eax, es:except_1
    mov idescr_14_31.offs_l, ax 
    shr eax, 16             
    mov idescr_14_31.offs_h, ax 

    
    lea eax, es:new_int08
    mov int08.offs_l, ax
    shr eax, 16
    mov int08.offs_h, ax

    lea eax, es:new_int09
    mov int09.offs_l, ax 
    shr eax, 16             
    mov int09.offs_h, ax 

    ; Получаем линейный адрес IDT
    mov ax, data16
    shl eax, 4
    add eax, offset idt

    ; Записываем в ipdescr линейный адрес IDT (Для з-р) 
    mov  dword ptr ipdescr + 2, eax
    ; И размер IDT
    mov  word ptr  ipdescr, idt_size-1 
    
    ; Сохранение масок
    in  al, 21h                     
    mov mask_master, al ; Ведущий.       
    in  al, 0A1h                    
    mov mask_slave, al  ; Ведомый.
    
    ; Перепрограммирование ведущего контроллера
    mov al, 11h
    out 20h, al                     
    mov al, 32 ; это новый базовый вектор (был до этого 8)
    out 21h, al                     
    mov al, 4
    out 21h, al
    mov al, 1
    out 21h, al

    ; Маска для ведущего контроллера
    mov al, 0FCh ; 1111 1100 - разрешаем только IRQ0 И IRQ1 (Interruption Request - Запрос прерывания)
    out 21h, al

    ; Маска для ведомого контроллера (запрещаем прерывания)
    mov al, 0FFh ; 1111 1111 - запрещаем все!
    out 0A1h, al
	
	; открытие линии A20 (если не откроем, то будут битые адреса, будет пропадать 20ый бит)
	mov al,0D1h
	out 64h,al
	mov al,0DFh
	out 60h,al
	
	;in  al, 92h
    ;or  al, 2
    ;out 92h, al
	
		
	cli; Запрет аппаратных прерываний. (Маскируемых)
    ; (Если мы это не сделаем, то в реальном режиме
    ; При возникновении первого же прерывания (к примеру от таймера)
    ; это привело бы к отключению процессора (т.к. IDTR указывает не туда))

	; lidt - load IDT - загрузить в регистр IDTR 
    ; Наш псевдодискриптор ipdescr, который содержит
    ; Лин адрес таблицы IDT и её размер.
    lidt fword ptr ipdescr
	
	; Запрет немаскируемых прерываний. NMI
    mov al, 80h
    out 70h, al
	
    ; Переход в защищенный режим
	mov		EAX,CR0
	or		EAX,1
	mov		CR0,EAX	

    db  66h  ; Префикс изменения разрядности операнда.
    db  0EAh ; Код команды far jmp.
    dd  offset pm_start ; Смещение
    dw  code32s         ; Сегмент
	
return_rm:
    ; возвращаем флаг pe
    mov eax, cr0
    and al, 0FEh                
    mov cr0, eax

    db  0EAh    
    dw  offset go
    dw  code16

go:
    ; обновляем все сегментные регистры
    mov ax, data16   
    mov ds, ax
    mov ax, code32
    mov es, ax
    mov ax, stack32   
    mov ss, ax
    mov ax, stack_size
    mov sp, ax
	
	; возвращаем базовый вектор контроллера прерываний
    mov al, 11h
    out 20h, al
    mov al, 8
    out 21h, al
    mov al, 4
    out 21h, al
    mov al, 1
    out 21h, al

    ; восстанавливаем маски контроллеров прерываний
    mov al, mask_master
    out 21h, al
    mov al, mask_slave
    out 0A1h, al

    ; восстанавливаем вектор прерываний (на 1ый кб)
    lidt    fword ptr ipdescr16
	
	; закрытие линии A20 (если не закроем, то сможем адресовать еще 64кб памяти (HMA, см. сем))
	mov al,0D1h
	out 04h,al
	mov al,0DDh
	out 60h,al
	
    ;in  al, 70h 
    ;and al, 7Fh
    ;out 70h, al

    sti ; Резрешаем (аппаратные) прерывания     
	
	; Резрешение немаскируемых прерываний. NMI
	xor al,al
	out 70h,al
	
    call ClearScreen
	
	mov ah, 09h
    lea dx, pm_msg_out
    int 21h
    call NewLine
		
    ; Завершаем программу.
	mov AX,4C00h
	int 21h	
		
	code16_size=$-main
code16 ends
end main