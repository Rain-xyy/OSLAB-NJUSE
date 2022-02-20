
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
			      console.c
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
						    Forrest Yu, 2005
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

/*
	回车键: 把光标移到第一列
	换行键: 把光标前进到下一行
*/

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

PRIVATE void set_cursor(unsigned int position);
PRIVATE void set_video_start_addr(u32 addr);
PRIVATE void flush(CONSOLE *p_con);

unsigned int lineFeed[(V_MEM_SIZE >> 1) / SCREEN_WIDTH][2]; //保存换行前后的cursor,0位表示上一行位置对应的下标，1位表示当前行位置对应的下标
int lfIdx = -1;

unsigned int tab[(V_MEM_SIZE >> 1) / 4]; //保存输入tab后的光标的位置
int tabIdx = -1;

unsigned char searchTxt[SCREEN_SIZE];
int searchTxtIdx = -1;

unsigned char input[SCREEN_SIZE][2]; //0位用来记录操作，1位用来记录每一个操作对应地模式(查找/非查找)
int inputIdx = -1;

/*======================================================================*
			   init_screen
 *======================================================================*/
PUBLIC void init_screen(TTY *p_tty)
{
	int nr_tty = p_tty - tty_table;
	p_tty->p_console = console_table + nr_tty;

	int v_mem_size = V_MEM_SIZE >> 1; /* 显存总大小 (in WORD) */

	int con_v_mem_size = v_mem_size / NR_CONSOLES;
	p_tty->p_console->original_addr = nr_tty * con_v_mem_size;
	p_tty->p_console->v_mem_limit = con_v_mem_size;
	p_tty->p_console->current_start_addr = p_tty->p_console->original_addr;
	p_tty->p_console->isSearch = 0;
	p_tty->p_console->closed = 0;

	//保存每一个输入的字符，该区域的总大小为该console的分配的显存的大小

	/* 默认光标位置在最开始处 */
	p_tty->p_console->cursor = p_tty->p_console->original_addr;

	if (nr_tty == 0)
	{
		/* 第一个控制台沿用原来的光标位置 */
		p_tty->p_console->cursor = disp_pos / 2;
		disp_pos = 0; //事实上有了cursor之后就不需要disp_pos了，之前用它来确定下一个字符在屏幕上的位置，现在就可以直接用cursor来确定了
	}
	else
	{
		out_char(p_tty->p_console, nr_tty + '0');
		out_char(p_tty->p_console, '#');
	}

	clean_screen(p_tty);
/* 	set_cursor(p_tty->p_console->cursor);
 */}

PUBLIC void clean_screen(TTY *p_tty)
{
	u8 *p_vmem = (u8 *)(V_MEM_BASE);
	for (int i = p_tty->p_console->original_addr; i < p_tty->p_console->cursor; i++)
	{
		*p_vmem++ = ' ';
		*p_vmem++ = DEFAULT_CHAR_COLOR;
	}
	p_tty->p_console->cursor = p_tty->p_console->original_addr;				//0
	p_tty->p_console->current_start_addr = p_tty->p_console->original_addr; //0
	disp_pos = 0;
	inputIdx = -1;
	tabIdx = -1;
	lfIdx = -1;
	searchTxtIdx = -1;
	flush(p_tty->p_console);
}

/*======================================================================*
			   is_current_console
*======================================================================*/
PUBLIC int is_current_console(CONSOLE *p_con)
{
	return (p_con == &console_table[nr_current_console]);
}

/*======================================================================*
			   is_line_feed
*======================================================================*/
PRIVATE int is_line_feed(unsigned int cursor)
{
	if (lfIdx == -1)
	{
		return 0;
	}
	return cursor == lineFeed[lfIdx][1]; //判断当前下标是否等于最后一个换行符的下标
}

/*======================================================================*
			   is_tab
*======================================================================*/
PRIVATE int is_tab(unsigned int cursor)
{
	if (tabIdx == -1)
	{
		return 0;
	}
	return cursor == tab[tabIdx];
}

PRIVATE int match_tab(unsigned int cursor)
{
	for (int i = 0; i <= tabIdx; i++)
	{
		if (tab[i] == cursor)
		{
			return 1;
		}
	}
	return 0;
}

/*======================================================================*
			   do_search
*======================================================================*/
PRIVATE void do_serach(CONSOLE *p_con)
{
	if (searchTxtIdx == -1)
	{
		return;
	}

	int start_cursor = 0;
	u8 *p_vmem = (u8 *)(V_MEM_BASE + start_cursor * 2);
	int end_cursor = 0;
	while (*(p_vmem + 1) == DEFAULT_CHAR_COLOR) //默认颜色，说明当前还在待查找的字符中
	{
		end_cursor++;
		p_vmem += 2;
	}
	p_vmem = (u8 *)(V_MEM_BASE + start_cursor * 2); //p_vmem恢复到开头位置

	while (start_cursor < end_cursor)
	{
		int temp_cursor = start_cursor;
		u8 *temp_p_vmem = p_vmem;
		for (int i = 0; i <= searchTxtIdx; i++)
		{
			if (match_tab(temp_cursor + 4))
			{
				if (searchTxt[i] != '\t')
				{
					start_cursor += 4;
					p_vmem += 8;
					break;
				}
			}
			if (searchTxt[i] == '\t')
			{
				if (!match_tab(temp_cursor + 4))
				{
					start_cursor++;
					p_vmem += 2;
					break;
				}
				temp_cursor += 4;
				temp_p_vmem += 8;

				if (i == searchTxtIdx)
				{
					while (start_cursor != temp_cursor)
					{
						start_cursor++;
						*(++p_vmem) = SEARCH_CHAR_COLOR;
						++p_vmem;
					}
				}
				if (temp_cursor >= end_cursor)
				{
					return;
				}
			}
			else
			{
				//正常匹配就好
				char ch = *temp_p_vmem; //取当前字符
				if (ch != searchTxt[i])
				{
					//当前字符不匹配，直接下一个字符重开
					start_cursor++;
					p_vmem += 2;
					break;
				}
				temp_cursor++;
				temp_p_vmem += 2;

				if (i == searchTxtIdx)
				{
					while (start_cursor != temp_cursor)
					{
						start_cursor++;
						*(++p_vmem) = SEARCH_CHAR_COLOR;
						++p_vmem;
					}
				}
				if (temp_cursor >= end_cursor)
				{
					return;
				}
			}
		}
	}
}

/*======================================================================*
			   recover
*======================================================================*/
PUBLIC void recover(TTY *p_tty)
{
	//恢复白色
	u8 *vmem_start = (u8 *)(V_MEM_BASE);
	u8 *vmem_end = (u8 *)(V_MEM_BASE + (p_tty->p_console->cursor - searchTxtIdx - 1) * 2 - 1);
	for (; vmem_start <= vmem_end;)
	{
		*(++vmem_start) = DEFAULT_CHAR_COLOR;
		++vmem_start;
	}
	//删除查找模式输入的关键字
	while (searchTxtIdx >= 0)
	{
		inputIdx--;
		out_char(p_tty->p_console, '\b');
	}
}

/*======================================================================*
			   revoke
*======================================================================*/
PUBLIC void revoke(TTY *p_tty)
{
	if (inputIdx < 0)
	{
		return;
	}

	char ch = input[inputIdx][0];
	int isSearch = input[inputIdx][1];
	CONSOLE *p_con = p_tty->p_console;
	u8 *p_vmem = (u8 *)(V_MEM_BASE + p_con->cursor * 2);

	if (p_tty->p_console->isSearch)
	{
		if (!isSearch)
		{
			return;
		}
	}

	if (!p_tty->p_console->isSearch)
	{
		if (isSearch)
		{
			inputIdx--;
			revoke(p_tty);
			return;
		}
	}

	inputIdx--;

	switch (ch)
	{
	case '\n':

		if (p_con->cursor < p_con->original_addr +
								p_con->v_mem_limit - SCREEN_WIDTH)
		{
			lfIdx++;
			lineFeed[lfIdx][0] = p_con->cursor;

			p_con->cursor = p_con->original_addr + SCREEN_WIDTH *
													   ((p_con->cursor - p_con->original_addr) /
															SCREEN_WIDTH +
														1);
			lineFeed[lfIdx][1] = p_con->cursor;
		}

		break;
	case '\b':
		if (p_con->cursor > p_con->original_addr)
		{

			if (is_line_feed(p_con->cursor))
			{
				p_con->cursor = lineFeed[lfIdx][0];
				lfIdx--;
			}
			else if (is_tab(p_con->cursor))
			{
				p_con->cursor -= 4;
				tabIdx--;
				if (isSearch)
				{
					searchTxtIdx--;
				}
			}
			else
			{
				if (p_con->cursor > p_con->original_addr)
				{
					p_con->cursor--;
					*(p_vmem - 2) = ' ';
					*(p_vmem - 1) = DEFAULT_CHAR_COLOR;
				}

				if (isSearch)
				{
					if (searchTxtIdx >= 0)
					{
						searchTxtIdx--;
					}
				}
			}
		}
		break;
	case '\t': //Tab键
		if (p_con->cursor <
			p_con->original_addr + p_con->v_mem_limit - 4)
		{

			for (int i = 0; i < 4; i++)
			{
				*p_vmem++ = ' ';
				if (isSearch)
				{
					*p_vmem++ = SEARCH_CHAR_COLOR;
				}
				else
				{
					*p_vmem++ = DEFAULT_CHAR_COLOR;
				}
			}
			p_con->cursor += 4;
			tabIdx++;
			tab[tabIdx] = p_con->cursor;
			if (isSearch)
			{

				searchTxtIdx++;
				searchTxt[searchTxtIdx] = '\t'; //将搜索的文本保存
			}
		}

		break;
	default:
		if (p_con->cursor <
			p_con->original_addr + p_con->v_mem_limit - 1)
		{
			*p_vmem++ = ch;
			if (isSearch)
			{
				searchTxtIdx++;
				searchTxt[searchTxtIdx] = ch; //将搜索的文本保存
				*p_vmem++ = SEARCH_CHAR_COLOR;
			}
			else
			{
				*p_vmem++ = DEFAULT_CHAR_COLOR;
			}
			p_con->cursor++;
		}
		break;
	}

	while (p_con->cursor >= p_con->current_start_addr + SCREEN_SIZE)
	{
		scroll_screen(p_con, SCR_DN);
	}

	while (p_con->cursor < p_con->current_start_addr)
	{
		scroll_screen(p_con, SCR_UP);
	}

	p_vmem = (u8 *)(V_MEM_BASE + p_con->cursor * 2);
	*(p_vmem + 1) = DEFAULT_CHAR_COLOR;

	flush(p_con);
}

/*======================================================================*
			   out_char
 *======================================================================*/
PUBLIC void out_char(CONSOLE *p_con, char ch)
{
	u8 *p_vmem = (u8 *)(V_MEM_BASE + p_con->cursor * 2);

	switch (ch)
	{
	case '\n':
		if (p_con->isSearch)
		{
			p_con->closed = 1; //屏蔽除ESC外的输入
			do_serach(p_con);
		}
		else
		{
			if (p_con->cursor < p_con->original_addr +
									p_con->v_mem_limit - SCREEN_WIDTH)
			{
				lfIdx++;
				lineFeed[lfIdx][0] = p_con->cursor;

				p_con->cursor = p_con->original_addr + SCREEN_WIDTH *
														   ((p_con->cursor - p_con->original_addr) /
																SCREEN_WIDTH +
															1);
				lineFeed[lfIdx][1] = p_con->cursor;

				//记录下操作，便于control-z撤回操作
				inputIdx++;
				input[inputIdx][0] = '\b';
				input[inputIdx][1] = p_con->isSearch;
			}
		}
		break;
	case '\b':
		if (p_con->cursor > p_con->original_addr)
		{
			if (is_line_feed(p_con->cursor))
			{
				p_con->cursor = lineFeed[lfIdx][0];
				lfIdx--;
				inputIdx++;
				input[inputIdx][1] = p_con->isSearch;
				input[inputIdx][0] = '\n';
			}
			else if (is_tab(p_con->cursor))
			{
				if (p_con->isSearch)
				{
					if (searchTxtIdx >= 0)
					{
						searchTxtIdx -= 1;
						p_con->cursor -= 4;
						tabIdx--;
						inputIdx++;
						input[inputIdx][1] = p_con->isSearch;
						input[inputIdx][0] = '\t';
					}
				}
				else
				{
					p_con->cursor -= 4;
					tabIdx--;
					inputIdx++;
					input[inputIdx][1] = p_con->isSearch;
					input[inputIdx][0] = '\t';
				}
			}
			else
			{

				if (p_con->isSearch)
				{
					if (searchTxtIdx >= 0)
					{
						inputIdx++;
						input[inputIdx][1] = p_con->isSearch;
						input[inputIdx][0] = *(p_vmem - 2);
						p_con->cursor--;
						*(p_vmem - 2) = ' ';
						*(p_vmem - 1) = DEFAULT_CHAR_COLOR;
						searchTxtIdx--;
					}
				}
				else
				{
					inputIdx++;
					input[inputIdx][1] = p_con->isSearch;
					input[inputIdx][0] = *(p_vmem - 2);
					p_con->cursor--;
					*(p_vmem - 2) = ' ';
					*(p_vmem - 1) = DEFAULT_CHAR_COLOR;
				}
			}
		}
		break;
	case '\t': //Tab键
		if (p_con->cursor <
			p_con->original_addr + p_con->v_mem_limit - 4)
		{
			//记录下操作，便于control-z撤回操作
			inputIdx++;
			input[inputIdx][0] = '\b';
			input[inputIdx][1] = p_con->isSearch;
			if (p_con->isSearch)
			{
				for (int i = 0; i < 4; i++)
				{
					*p_vmem++ = ' ';
					*p_vmem++ = DEFAULT_CHAR_COLOR;
				}
			}
			else
			{
				for (int i = 0; i < 4; i++)
				{
					*p_vmem++ = ' ';
					*p_vmem++ = DEFAULT_CHAR_COLOR;
				}
			}

			p_con->cursor += 4;
			tabIdx++;
			tab[tabIdx] = p_con->cursor;
			if (p_con->isSearch)
			{
				/* for (int i = 0; i < 4; i++)
				{
					searchTxtIdx++;
					searchTxt[searchTxtIdx] = ' '; //将搜索的文本保存
				} */
				searchTxtIdx++;
				searchTxt[searchTxtIdx] = '\t'; //将搜索的文本保存
			}
		}
		break;
	default:
		if (p_con->cursor <
			p_con->original_addr + p_con->v_mem_limit - 1)
		{
			//记录下操作，便于control-z撤回操作
			inputIdx++;
			input[inputIdx][0] = '\b';
			input[inputIdx][1] = p_con->isSearch;

			*p_vmem++ = ch;
			if (p_con->isSearch)
			{
				searchTxtIdx++;
				searchTxt[searchTxtIdx] = ch; //将搜索的文本保存
				*p_vmem++ = SEARCH_CHAR_COLOR;
			}
			else
			{
				*p_vmem++ = DEFAULT_CHAR_COLOR;
			}
			p_con->cursor++;
		}
		break;
	}

	while (p_con->cursor >= p_con->current_start_addr + SCREEN_SIZE)
	{
		scroll_screen(p_con, SCR_DN);
	}

	while (p_con->cursor < p_con->current_start_addr)
	{
		scroll_screen(p_con, SCR_UP);
	}

	p_vmem = (u8 *)(V_MEM_BASE + p_con->cursor * 2);
	*(p_vmem + 1) = DEFAULT_CHAR_COLOR;

	flush(p_con);
}

/*======================================================================*
                           flush
*======================================================================*/
PRIVATE void flush(CONSOLE *p_con)
{
	set_cursor(p_con->cursor);
	set_video_start_addr(p_con->current_start_addr);
}

/*======================================================================*
			    set_cursor
 *======================================================================*/
PRIVATE void set_cursor(unsigned int position)
{
	disable_int();
	out_byte(CRTC_ADDR_REG, CURSOR_H);
	out_byte(CRTC_DATA_REG, (position >> 8) & 0xFF);
	out_byte(CRTC_ADDR_REG, CURSOR_L);
	out_byte(CRTC_DATA_REG, position & 0xFF);
	enable_int();
}

/*======================================================================*
			  set_video_start_addr
 *======================================================================*/
PRIVATE void set_video_start_addr(u32 addr)
{
	disable_int();
	out_byte(CRTC_ADDR_REG, START_ADDR_H);
	out_byte(CRTC_DATA_REG, (addr >> 8) & 0xFF);
	out_byte(CRTC_ADDR_REG, START_ADDR_L);
	out_byte(CRTC_DATA_REG, addr & 0xFF);
	enable_int();
}

/*======================================================================*
			   select_console
 *======================================================================*/
PUBLIC void select_console(int nr_console) /* 0 ~ (NR_CONSOLES - 1) */
{
	if ((nr_console < 0) || (nr_console >= NR_CONSOLES))
	{
		return;
	}

	nr_current_console = nr_console;

	set_cursor(console_table[nr_console].cursor);
	set_video_start_addr(console_table[nr_console].current_start_addr);
}

/*======================================================================*
			   scroll_screen
 *----------------------------------------------------------------------*
 滚屏.
 *----------------------------------------------------------------------*
 direction:
	SCR_UP	: 向上滚屏
	SCR_DN	: 向下滚屏
	其它	: 不做处理
 *======================================================================*/
PUBLIC void scroll_screen(CONSOLE *p_con, int direction)
{
	if (direction == SCR_UP)
	{
		if (p_con->current_start_addr > p_con->original_addr)
		{
			p_con->current_start_addr -= SCREEN_WIDTH;
		}
	}
	else if (direction == SCR_DN)
	{
		if (p_con->current_start_addr + SCREEN_SIZE <
			p_con->original_addr + p_con->v_mem_limit)
		{
			p_con->current_start_addr += SCREEN_WIDTH;
		}
	}
	else
	{
	}

	set_video_start_addr(p_con->current_start_addr);
	set_cursor(p_con->cursor);
}
