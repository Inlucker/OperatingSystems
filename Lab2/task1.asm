.586P	

descr	struc
	lim		dw	0
	base_l	dw	0
	base_m	db	0
	attr_1	db	0
	attr_2	db	0
	base_h	db	0
descr	ends

; Сегмент стека.
stack32 segment  para stack 'STACK'
    stack_start db  100h dup(?)
    stack_size = $-stack_start
stack32 ends

data16 segment para 'data' use16
    gdt_null  descr <>
	; дескриптор, описывающий сегмент для реального режима (там типа все в большинстве своем 16 битное, поэтому лучше и описать сегмент как 16 битный)
	gdt_code16 descr <code16_size-1,0,0,98h,0,0>
    gdt_data16 descr <data16_size-1,0,0,92h,0,0>
	; дескриптор сегмента кода с 32 битными операциями (там описаны обработчики прерываний в защищенном режиме, код защищенного режима и тп (потому что они используют преимущественно 32 битные операнды))
    gdt_code32 descr <code32_size-1,0,0,98h,40h,0>
    ; дескриптор для измерения памяти (описывает сегмент размера 4гб и начало которого на 0 байте)
	gdt_data32 descr <0FFFFh,0,0,92h,0CFh,0> ; fffff = 2^20 и по страницам, т.е. (G - бит гранулярности) т.е. по 4Кл = 4*2^10 ==> 2^20 * 4 * 2^10 = 4*2^30
    gdt_stack32 descr <stack_size-1,0,0,92h,40h,0>
	; Размер видеостраницы составляет 4000 байт - поэтому граница 3999.
    ; B8000h - базовый физический адрес страницы (8000h и 0Bh). 
    ; (Видео память размещена в первом Мегабайте адр. пр-ва поэтому base_m = 0).
	gdt_screen16 descr <3999,8000h,0Bh,92h,0,0> ; 0 + B 0000h + 8000h = 0B8000h 
	
    ; Размер таблицы GDT.
	gdt_size=$-gdt_null
	
	; DF - выделить поле для псевдодескриптора (6-байт).
	pdescr	df	0
	
	; Селекторы - номер (индекс начала) дескриптора в GDT.
    code16s=8
    data16s=16
    code32s=24
    data32s=32
    stack32s=40
    video16s=48
	
	mem_pos=0 
    ; позиция на экране значения кол-ва доступной памяти (имеется ввиду то, что после `Memory:`)
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
	
    ; Выход из защищенного режима
    db  0EAh ; jmp
    dd  offset return_rm ; offset
    dw  code16s ; selector
	
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

        ; Это оставшееся FFEF FFFE + 10 0001 = F0000 0000 ==  (2^4)^8 = 2^32  = 4 Гб
        mov ecx, 0FFEFFFFEh

        ; Из методы:
        ; В защищенном режиме определить объем доступной физической
        ; памяти следующим образом – первый мегабайт пропустить;
        ; начиная со второго мегабайта сохранить байт или слово памяти, 
        ; записать в этот байт или слово сигнатуру, прочитать сигнатуру и 
        ; сравнить с сигнатурой в программе, если сигнатуры совпали, то это – память.
        ; Вывести на экран полученной количество байтов доступной памяти.

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
        ; Деедим на 2^20 (кол-во байт в мегабайте).
        mov ebx, 100000h ; 16^5 = (2^4)^5 = 2^20
        div ebx ; делим eax / ebx -> eax содержит кол-во МЕГАБАЙТ.

        mov ebx, mem_value_pos
        ; функция, которая печатает eax (в котором лежит найденное кол-во мегабайт)
        call print_count_memory
        ; Печать надписи Mb (мегабайты)
        mov ah, param
        mov ebx, mb_pos
        mov al, 'M'
        mov es:[ebx], ax

        mov ebx, mb_pos + 2
        mov al, 'b'
        mov es:[ebx], ax
        ret

    count_memory_proc endp
    
; FFFF FFFF - 4 байта
; FF - 1 байт

    print_count_memory proc uses ecx ebx edx
        ; В eax лежит кол-во мегабайт.
        ; В ebx лежит mem_value_pos.
        ; add ebx, 10h ; сдвигаем ebx на 8 позиций (будем печатать 8 символов)
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
            ; Добавляем к 'A' dl, то есть то, что больше 10
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
		
NewLine: 
    ; Перенес на новую строку.
    xor dx, dx
    mov ah, 2  ; Номер команды, для вывода символа. 
    mov dl, 13 ; Возврат коретки.
    int 21h    
    mov dl, 10 ; Перенос на новую строку.
    int 21h
    ret

ClearScreen:
    ; Инструкция очистки экрана
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
	
	; Коварный вопрос: размер GDTR? - 32 бита (4 байта)
    ; В Зубкове на стр 477 написано, что он 48 битный (6 байт)
    ; LGDT (Load GDT) - загружает в регистр процессора GDTR (GDT Register)  (лежит лин. адр этой табл и размер)
    ; (LGDT относится к типу привилегированных команд.)
    ; Вызываем ее в р-р.
    ; Это говорит нам о том, что в р-р нет никакой защиты.
    ;mov dword ptr pdescr+2, eax
    ;mov word ptr  pdescr, gdt_size-1
    ; fword - получается, что это 6 байт
    ;lgdt fword ptr pdescr
	
	mov	dword ptr pdescr+2,eax;
	mov	word ptr pdescr,gdt_size-1	
	lgdt pdescr
	;lgdt fword ptr pdescr
	
	mov ax, code32
    mov es, ax
		
	;Добавить IDTR
	
	; открытие линии A20 (если не откроем, то будут битые адреса, будет пропадать 20ый бит)
    in  al, 92h
    or  al, 2
    out 92h, al
		
	cli
	
    ; Переход в защищенный режим
	mov		EAX,CR0
	or		EAX,1
	mov		CR0,EAX	

	; Префикс 66h - говорит нам о том, что
    ; След. команда будет разрядностью, противоложной нашего сегмента (use16) 
    db  66h  ; Префикс изменения разрядности операнда (меняет на противоположный).
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
	
	; закрытие линии A20 (если не закроем, то сможем адресовать еще 64кб памяти (HMA, см. сем))
    in  al, 70h 
    and al, 7Fh
    out 70h, al

    sti ; Резрешаем (аппаратные) прерывания     
	
	;call ClearScreen
	
    call NewLine
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