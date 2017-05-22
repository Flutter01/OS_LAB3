
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                               tty.c
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                                                    Forrest Yu, 2005
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

#include "type.h"
#include "const.h"
#include "protect.h"
#include "string.h"
#include "proc.h"
#include "tty.h"
#include "console.h"
#include "global.h"
#include "keyboard.h"
#include "proto.h"

#define TTY_FIRST	(tty_table)
#define TTY_END		(tty_table + NR_CONSOLES)

PRIVATE void init_tty(TTY* p_tty);
PRIVATE void tty_do_read(TTY* p_tty);
PRIVATE void tty_do_write(TTY* p_tty);
PRIVATE void put_key(TTY* p_tty, u32 key);

extern int to_clear;
unsigned int tabs[100];
unsigned int tabSpace[100];
int tabs_len = 0;
int isTab = 0;
int backspace_len=0;
unsigned int backspace[100];
unsigned int backspace_loc[100];
extern int enterTag;//判断enter键是否是在ESC查询模式中显示结果，若entertag为1则屏蔽除ESC之外的其余输入
extern int input_mode;
extern int mode_length;
extern void find_string(CONSOLE* p_con);
extern void cleanColor(CONSOLE* p_con);
int space_num=0;
/*======================================================================*
                           task_tty
 *======================================================================*/
PUBLIC void task_tty()
{
	TTY*	p_tty;
	tabs_len = 0;
	init_keyboard();
        backspace_len=0;
        space_num=0;
	for (p_tty=TTY_FIRST;p_tty<TTY_END;p_tty++) {
		init_tty(p_tty);
	}
	select_console(0);
	while (1) {
		for (p_tty=TTY_FIRST;p_tty<TTY_END;p_tty++) {
			if (to_clear == 1 && is_current_console(p_tty->p_console)) {
				while(p_tty->p_console->cursor > 0 && input_mode == 0) {
					out_char(p_tty -> p_console, '\b');
					to_clear = 0;
				}
			}
			tty_do_read(p_tty);
			tty_do_write(p_tty);
		}
	}
}

/*======================================================================*
			   init_tty
 *======================================================================*/
PRIVATE void init_tty(TTY* p_tty)
{
	p_tty->inbuf_count = 0;
	p_tty->p_inbuf_head = p_tty->p_inbuf_tail = p_tty->in_buf;

	init_screen(p_tty);
}

/*======================================================================*
				in_process
 *======================================================================*/
PUBLIC void in_process(TTY* p_tty, u32 key)
{
        char output[2] = {'\0', '\0'};
        int it = 0;
        if (!(key & FLAG_EXT)) {
			if(enterTag==0)
		put_key(p_tty, key);
           
        }
        else {
                int raw_code = key & MASK_RAW;
        switch(raw_code) {
        case ENTER:
		if(enterTag==0){
			if (input_mode==0) {
				put_key(p_tty, '\n');
				enterTag=0;
			}else{
				enterTag=1;
				find_string(p_tty->p_console);
			}
		}
			break;
        case BACKSPACE:
		if(enterTag==0){
			put_key(p_tty, '\b');
			}
			break;
			
        case UP:
		if(enterTag==0){
            if ((key & FLAG_SHIFT_L) || (key & FLAG_SHIFT_R)) {
				scroll_screen(p_tty->p_console, SCR_DN);
            }
			}
			break;
			
		case DOWN:
		if(enterTag==0){
			if ((key & FLAG_SHIFT_L) || (key & FLAG_SHIFT_R)) {
				scroll_screen(p_tty->p_console, SCR_UP);
			}
			}
			break;
			
		case TAB: 
		if(enterTag==0){
                     
                        int yushu =p_tty->p_console->cursor%SCREEN_WIDTH;
                        if((SCREEN_WIDTH-yushu)>4){
                        int number_of_four=yushu%4;
                         space_num=4-number_of_four;
                   
                         }
                       else{
                         space_num=SCREEN_WIDTH-yushu+4;
                        }

			for(it = 0; it < space_num; it++){
                                isTab = 1;
				put_key(p_tty, ' ');
               
                       }                 
                        tabSpace[tabs_len]=space_num;
			tabs[tabs_len] = p_tty->p_console->cursor;
                         tabs_len++;
                         isTab=0;
			
}
			break;
		case ESC:
			if (input_mode == 0) {
				mode_length = 0;
				input_mode = 1;
			} else {
				cleanColor(p_tty->p_console);
				for(it = 0; it < mode_length; it++) {
					out_char(p_tty -> p_console, '\b');
				}
				input_mode = 0;
				enterTag =0;
			}
			break;
		case F1:
		case F2:
		case F3:
		case F4:
		case F5:
		case F6:
		case F7:
		case F8:
		case F9:
		case F10:
		case F11:
		case F12:
			/* Alt + F1~F12 */
			if(enterTag==0){
			if ((key & FLAG_ALT_L) || (key & FLAG_ALT_R)) {
				select_console(raw_code - F1);
			}
			}
			break;
                default:
                        break;
                }
        }
}

/*======================================================================*
			      put_key
*======================================================================*/
PRIVATE void put_key(TTY* p_tty, u32 key)
{
	if (p_tty->inbuf_count < TTY_IN_BYTES) {
		*(p_tty->p_inbuf_head) = key;
		p_tty->p_inbuf_head++;
		if (p_tty->p_inbuf_head == p_tty->in_buf + TTY_IN_BYTES) {
			p_tty->p_inbuf_head = p_tty->in_buf;
		}
		p_tty->inbuf_count++;
	}
}


/*======================================================================*
			      tty_do_read
 *======================================================================*/
PRIVATE void tty_do_read(TTY* p_tty)
{
	if (is_current_console(p_tty->p_console)) {
		keyboard_read(p_tty);
	}
}


/*======================================================================*
			      tty_do_write
 *======================================================================*/
PRIVATE void tty_do_write(TTY* p_tty)
{
	if (p_tty->inbuf_count) {
		char ch = *(p_tty->p_inbuf_tail);
		p_tty->p_inbuf_tail++;
		if (p_tty->p_inbuf_tail == p_tty->in_buf + TTY_IN_BYTES) {
			p_tty->p_inbuf_tail = p_tty->in_buf;
		}
		p_tty->inbuf_count--;

		out_char(p_tty->p_console, ch);
	}
}


