/* 
 * fixed bad-addi-constant bug in hextoi        12-Jan-03 kenmac
 * added push,pop (S-type)                      11-Jan-04 malcolm
 * added ei,di,ret (M-type)             Fri Jan 30 01:08:38 EST 2004 malcolm
 *
 * added la
 * changed reg order in instr					21-Dec-12 kelly
 * to be like prj1
 *
 * RCS: $Id$
 * assembler for CS2200, LC-2003
 */

#include <stdio.h>
#include <stdlib.h>             /* strtol() */
#include <string.h>             /* strtok() */
#include <assert.h>

/*
 * 0. global state (gross, but handy for consistent error reporting)
 */
static char *sourcefilename = "<none>";
static int sourcelinenumber = 0;
static int nerrors = 0;

#define MAXLINELEN 1024
#define MAXTOKENS 64
#define COMMENTCHAR '!'         /* comments between ! and end-of-line */
#define OTHERCOMMENTCHAR '#'    /* MIPS uses '#' */
#define OTHEROTHERCOMMENTCHAR ';' /*students want to use ; as well */
#define OUTEXTENSION ".lc"      /* extention on the output file. */

/*
 * 1. representation of a single instruction 
 */
#define OP_BITS 4               /* 4-bit opcodes */
#define REG_BITS 4              /* 16 registers */
#define OFFSET_BITS 20          /* 16-bit signed offset */

#define OP_OFFSET 28            /* position in the instruction word */
#define OFFSET_OFFSET 0
#define X_OFFSET 24
#define Y_OFFSET 20
#define Z_OFFSET 0

#define NREGS (1 << REG_BITS)   /* i.e. 16 */

enum {
	OP_ADD = 0,
	OP_NAND,
	OP_ADDI,
	OP_LW,
	OP_SW,
	OP_BEQ,
	OP_JALR,
	OP_HALT,
	OP_BONR,
	OP_BONO,
	OP_EI,
	OP_DI,
	OP_RETI,
	OP_BONI,
	OP_BONJ,
	OP_COUNT                    /* number of opcodes */
} op_code_t;

/*
 * 2. Descriptions of the instructions: the opcode mnemonic, the opcode
 *    value and the format, followed by a table of all the instructions.
 *    Pseudo-ops also go in this table.
 */
typedef struct {
	int opcode;
	char *mnemonic;
	enum { R, I, II, J, O, S, P, M } format;
} instruction_t;

#define NINSTRS 18              /* this is unrelated to # of opcodes */
static instruction_t instruction_table[NINSTRS] = {
	{OP_ADD, "add", R},
	{OP_NAND, "nand", R},
	{OP_ADDI, "addi", II},      /* instr format is I but syntax is odd */
	{OP_LW, "lw", I},
	{OP_SW, "sw", I},
	{OP_BEQ, "beq", II},
	{OP_JALR, "jalr", J},
	{OP_HALT, "halt", O},
	{OP_BONR, "bonr", R},        /* HACK HACK: noop emits add 0 0 0 */
	{OP_BONO, "bono", O},       /* HACK HACK: -1 hardwired offset for dec */
	{OP_EI, "ei", M},
	{OP_DI, "di", M},
	{OP_RETI, "reti", M},
	{OP_BONI, "boni", II},
	{OP_BONJ, "bonj", J},
	{-1, ".word", P},           /* pseudo-op */
	{-1, ".fill", P},            /* pseudo-op (same as .word) */
	{-1, "la", P}
};

static char *register_names[NREGS] = {
	"$zero",                    /* 0 */
	"$at",                      /* 1 */
	"$v0",                      /* 2 */
	"$a0",                      /* 3 */
	"$a1",                      /* 4 */
	"$a2",                      /* 5 */
	"$a3",                      /* 6 */
	"$a4",                      /* 7 */
	"$s0",                      /* 8 */
	"$s1",                      /* 9 */
	"$s2",                      /* 10 */
	"$s3",                      /* 11 */
	"$k0",                      /* 12 */
	"$sp",                      /* 13 */
	"$fp",                      /* 14 */
	"$ra"                       /* 15 */
};

/*
 * 3. symbol table entry.  The symbol table is stored
 *    in a single-linked list.
 */
typedef struct symbol {
	int value;
	char *name;
	struct symbol *link;
} symbol_t;

static symbol_t *symbol_table = NULL;

/*************************************************************\
 *                                                           *
 * Utility routine hextoi() for reading decimal/hex numbers. *
 *                                                           *
 \*************************************************************/

static long hextoi(char *string)
{
	long number = 0;
	int index;

	/*
	 * if it does NOT look like a hex number, assume decimal and use atoi.
	 */
	if (!(string[0] == '0' && (string[1] == 'x' || string[1] == 'X')))
		return (atoi(string));

	/*
	 * otherwise, read a hex number.
	 */
	for (index = 2; string[index] != '\0'; index++) {
		char blah = string[index];
		int digit;

		if (blah >= '0' && blah <= '9')
			digit = blah - '0';
		else if (blah >= 'A' && blah <= 'F')
			digit = blah - 'A' + 10;
		else if (blah >= 'a' && blah <= 'f')
			digit = blah - 'a' + 10;
		else
			break;

		number = number * 16 + digit;
	}
	return (number);
}

/*********************************************************************\
 *                                                                   *
 * getLine reads in a single line from the given file.  It returns a *
 * pointer to the line (in static storage) or NULL on EOF.           *
 *                                                                   *
 \*********************************************************************/

static char *getLine(FILE * input)
{
	static char line[MAXLINELEN];
	int kount;
	int blah;

	for (kount = 0; kount < MAXLINELEN - 1; kount++) {
		blah = getc(input);
		if (blah == EOF || blah == '\n')
			break;
		line[kount] = blah;
	}
	line[kount] = '\0';
	if (kount == 0 && blah == EOF)
		return (NULL);
	return (line);
}

/***********************************************************************\
 *                                                                     *
 * lexer takes a string representing a line and returns an ARGV-style  *
 * array of pointers to strings representing tokens.  The array is in  *
 * internal, static storage.                                           *
 *                                                                     *
 * 1. Everything after a "!" character is removed.                     *
 * 2. The last entry in the pointer array is NULL                      *
 * 3. The input string is preserved.                                   *
 *                                                                     *
 * For example:                                                        *
 *                                                                     *
 *         char **argv = lexer("this is a test     ! that was a test") *
 *                                                                     *
 *   returns an array like this:                                       *
 *                                                                     *
 *         argv[0] = "this"                                            *
 *         argv[1] = "is"                                              *
 *         argv[2] = "a"                                               *
 *         argv[3] = "test"                                            *
 *         argv[4] = ""                                                *
 *                                                                     *
 *   and remaining entries are undefined.                              *
 *                                                                     *
 \***********************************************************************/

static char *colon = ":";
static char *comma = ",";
static char *lparen = "(";
static char *rparen = ")";

static int is_whitespace(char blah)
{
	return (blah == ' ' || blah == '\t');
}

static int is_rawdelimiter(char blah)
{
	return (blah == ':' || blah == ',' || blah == '(' || blah == ')');
}

static int is_eol(char blah)
{
	return (blah == '\0' || blah == COMMENTCHAR || blah == OTHERCOMMENTCHAR
			|| blah == OTHEROTHERCOMMENTCHAR);
}

static char **lexer(char *input_line)
{
	static char line[MAXLINELEN];
	static char *tokens[MAXTOKENS];
	int kount, index;

	/*
	 * duplicate the line since strtok is going to change it and
	 * it's useful to keep a copy for error messages.  While we're
	 * at it, strip any comment.
	 */
	for (index = 0; index < MAXLINELEN - 1; index++) {
		if (is_eol(input_line[index]))
			break;
		line[index] = input_line[index];
	}
	line[index] = '\0';

	/*
	 * split the copy of the line into tokens.  Note we modify
	 * the line in-place and the tokens[] array ends up pointing into
	 * pieces of the line.
	 */
	for (kount = 0, index = 0; kount < MAXTOKENS - 1; kount++) {
		while (is_whitespace(line[index]))  /* zero the whitespace */
			line[index++] = '\0';

		if (is_eol(line[index])) {  /* zero the EOL as well. */
			line[index] = '\0';
			break;
		}

		if (is_rawdelimiter(line[index])) {
			tokens[kount] = (line[index] == ',' ? comma /* point to delim. */
					: line[index] == ':' ? colon
					: line[index] == '(' ? lparen : rparen);
			line[index++] = '\0';
		} else {
			tokens[kount] = &line[index];   /* point into fragment of line */
			while (!is_eol(line[index])
					&& !is_whitespace(line[index])
					&& !is_rawdelimiter(line[index]))
				index++;
		}
	}

	/*
	 * add a blank token to mark end-of-tokens
	 */
	tokens[kount] = NULL;
	return (tokens);
}

/************************************************************************\
 *                                                                      *
 * Symbol_lookup looks up a symbol in the symbol table.  It takes a     *
 * string representing the name as an argument and returns a pointer to *
 * the symbol structure or NULL.                                        *
 *                                                                      *
 \************************************************************************/

static symbol_t *symbol_lookup(char *name)
{
	symbol_t *symbol;

	for (symbol = symbol_table; symbol; symbol = symbol->link)
		if (strcmp(symbol->name, name) == 0)
			break;
	return (symbol);
}

/***********************************************************************\
 *                                                                     *
 * symbol_insert adds a symbol to the symbol table.  Duplicate symbols *
 * are detected.  Input is a symbol and a proposed value.  A one is    *
 * returned on success and a zero if a duplicate is detected.          *
 *                                                                     *
 \***********************************************************************/

static int symbol_insert(char *name, int value)
{
	symbol_t *symbol;

	/*
	 * test for an existing symbol
	 */
	if (symbol_lookup(name) != NULL)
		return (0);

	/*
	 * allocate symbol and link it into the symbol table (singly-linked list)
	 */
	symbol = (symbol_t *) malloc(sizeof(symbol_t));
	symbol->name = strdup(name);
	symbol->value = value;
	symbol->link = symbol_table;
	symbol_table = symbol;
	return (1);
}
#ifdef DEBUG
/*******************************\
 *                             *
 * symbol_dump: debugging aid. *
 *                             *
 \*******************************/

static void symbol_dump(FILE * output)
{
	char buffer[MAXLINELEN];
	symbol_t *symbol;

	if (symbol_table == NULL) {
		fprintf(output, "\t\t!\n");
		fprintf(output, "\t\t! No symbols used.\n");
	} else {
		fprintf(output, "\t\t!\n");
		fprintf(output, "\t\t! Symbol table:\n");
		fprintf(output, "\t\t!\n");
		for (symbol = symbol_table; symbol; symbol = symbol->link) {
			sprintf(buffer, "\"%s\"", symbol->name);
			fprintf(output, "\t\t! %-20s = 0x%08X, %d\n",
					buffer, symbol->value, symbol->value);
		}
	}
}
#endif
/****************************************************************************\
 *                                                                          *
 * first_pass reads the input file filling in the symbols only.  The only   *
 * error checking performed on this pass is checking for duplicate symbols. *
 * input is the file, return a one on succes and a zero on failure.         *
 *                                                                          *
 \****************************************************************************/

static int is_delimiter(char *token)
{
	return (token == comma
			|| token == colon || token == lparen || token == rparen);
}

static int first_pass(FILE * input)
{
	int pc = 0;                 /* program counter for symbol values */
	char *line;                 /* whole input line */
	char **tokens;              /* ARGV-style split of line */

	sourcelinenumber = 0;
	nerrors = 0;
	while ((line = getLine(input)) != NULL) {
		tokens = lexer(line);   /* convert line to tokens */
		sourcelinenumber++;     /* update line number for errors */

		/*
		 * blank line -- ignore it
		 */
		if (tokens[0] == NULL)
			continue;

		/*
		 * delimiter character as first token?!
		 */
		if (is_delimiter(tokens[0])) {
			fprintf(stderr,
					"%s:%d: bad delimiter character '%s' at begining of line\n",
					sourcefilename, sourcelinenumber, tokens[0]);
			nerrors++;
			continue;
		}

		/*
		 * second token is ':' delimiter -- assume the first token
		 * is a label and add it in with the value of the current PC.
		 */
		if (tokens[1] == colon) {
			if (!symbol_insert(tokens[0], pc)) {
				fprintf(stderr,
						"%s:%d: duplicate symbol '%s'\n",
						sourcefilename, sourcelinenumber, tokens[0]);
				nerrors++;
			}
			tokens++;           /* advance ptr to skip the label */
			tokens++;           /* advance ptr to skip the delimiter */
		}

		/*
		 * if the line (also) contains an opcode, bump the PC
		 */
		if (tokens[0] != NULL)
			pc++;
	}
	fprintf(stderr, "%d errors on the first pass\n", nerrors);
	return (nerrors == 0);      /* 1 on success, 0 on failure */
}

/**************************************************************************\
 *                                                                        *
 * lookup_instruction takes a string representing a mnemonic and looks it *
 * up in the instruction table. A pointer to a instruction structure is   *
 * returned on success, else NULL.                                        *
 *                                                                        *
 \**************************************************************************/

static instruction_t *lookup_instruction(char *mnemonic)
{
	int kount;

	for (kount = 0; kount < NINSTRS; kount++)
		if (strcmp(mnemonic, instruction_table[kount].mnemonic) == 0)
			return (&instruction_table[kount]);
	return (NULL);
}

/****************************************************************\
 *                                                              *
 * is_numeric returns true if a token looks like a valid number *
 * in decimal, hex or octal.                                    *
 *                                                              *
 \****************************************************************/

static int is_decimal_char(char blah)
{
	return (blah >= '0' && blah <= '9');
}

static int is_octal_char(char blah)
{
	return (blah >= '0' && blah <= '7');
}

static int is_hexadecimal_char(char blah)
{
	return (is_decimal_char(blah)
			|| (blah >= 'A' && blah <= 'F')
			|| (blah >= 'a' && blah <= 'f'));
}

static int is_numeric(char *token)
{
	int kount;

	assert(token[0] != '\0');   /* blank token impossible */
	if (token[0] == '-')        /* negative okay */
		token++;
	if (token[0] == '0') {
		/*
		 * a token beginning with 0 may be octal or hexadecimal
		 */
		if (token[1] == 'x' || token[1] == 'X') {
			/*
			 * begins with 0x or 0X: it's hexadecimal.  Confirm that
			 * it has at least one digit following the X and that
			 * all of the digits are in hex.
			 */
			if (token[2] == '\0')
				return (0);
			for (kount = 2; token[kount] != '\0'; kount++)
				if (!is_hexadecimal_char(token[kount]))
					return (0);
			return (1);
		} else {
			/*
			 * didn't begin with 0x, it's octal (or just a zero).  Confirm
			 * that the rest of the digits (if any) are octal.
			 */
			for (kount = 1; token[kount] != '\0'; kount++)
				if (!is_octal_char(token[kount]))
					return (0);
			return (1);
		}
	} else {                    /* i.e. token[0] != '0' */

		/*
		 * a token beginning with a non-zero is decimal (or garbage)
		 */
		for (kount = 0; token[kount] != '\0'; kount++)
			if (!is_decimal_char(token[kount]))
				return (0);
		return (1);
	}
}                               /* [end of is_numeric()] */

/***********************************************************************\
 *                                                                     *
 * check_length takes a ARGV-style array of pointers to strings        *
 * and tests that given number are non-null.  This is a helper         *
 * for the second pass.  Returns one on success, else prints a message *
 * and returns zero.                                                   *
 *                                                                     *
 \***********************************************************************/

static int check_length(char **tokens, int n, char *mnem)
{
	int nfound = 0;
	int kount;

	for (kount = 0; kount <= n; kount++)    /* check n+1 items */
		if (tokens[kount] != NULL)
			nfound++;
		else
			break;

	if (nfound != n) {
		fprintf(stderr,
				"%s:%d: wrong number of tokens (%d instead of %d) to %s\n",
				sourcefilename, sourcelinenumber, nfound, n, mnem);
		nerrors++;
	}
	return (nfound == n);
}

/*************************************************************************\
 *                                                                       *
 * token_to_regno converts a symbolic register name to a register number *
 * or prints an error message on failure.                                *
 *                                                                       *
 \*************************************************************************/

static int token_to_regno(char *token, char *mnem)
{
	static int register_help_message = 0;
	int kount;

	for (kount = 0; kount < NREGS; kount++)
		if (!strcmp(token, register_names[kount]))
			return (kount);

	fprintf(stderr,
			"%s:%d: Read '%s' when expecting a register #.  opcode %s\n",
			sourcefilename, sourcelinenumber, token, mnem);
	nerrors++;

	if (!register_help_message) {
		fprintf(stderr, "valid register names are:\n");
		for (kount = 0; kount < NREGS; kount++)
			fprintf(stderr, "  %s\n", register_names[kount]);
		register_help_message = 1;
	}

	return (0);
}

/******************************************\
 *                                        *
 * check_delimiter helps syntax checking. *
 *                                        *
 \******************************************/

static void check_delimiter(char *token, char *delimiter, char *mnem)
{
	if (token != delimiter) {
		fprintf(stderr,
				"%s:%d: Read '%s' when expecting a '%s'.  opcode is %s\n",
				sourcefilename, sourcelinenumber, token, delimiter, mnem);
		nerrors++;
	}
}

/************************************************************************\
 *                                                                      *
 * check_offset checks that the given offset fits in a two's complement *
 * number in OFFSET_BITS bits.  Return 1 if okay, else 0.               *
 *                                                                      *
 \************************************************************************/

static int check_offset(int offset, char *mnem)
{
	int range = (1 << (OFFSET_BITS - 1));   /* i.e. 128 */

	if (offset < -range) {
		fprintf(stderr,
				"%s:%d: Offset of %d is smaller than %d.  opcode is %s\n",
				sourcefilename, sourcelinenumber, offset, -range, mnem);
		nerrors++;
		return (0);
	} else if (offset >= range) {
		fprintf(stderr,
				"%s:%d: Offset of %d is greater than %d.  opcode is %s\n",
				sourcefilename, sourcelinenumber, offset, range - 1, mnem);
		nerrors++;
		return (0);
	}
	return (1);
}

/****************************************************\
 *                                                  *
 * token_to_number returns a number or symbol value *
 *                                                  *
 \****************************************************/

static int token_to_number(char *token, char *mnem)
{
	int number;

	if (is_numeric(token)) {
		number = hextoi(token);
	} else {
		symbol_t *symbol = symbol_lookup(token);

		if (symbol == NULL) {
			fprintf(stderr,
					"%s:%d: undefined symbol '%s'.  opcode is %s\n",
					sourcefilename, sourcelinenumber, token, mnem);
			nerrors++;
			number = 0;
		} else {
			number = symbol->value;
		}
	}
	return (number);
}

/***********************************************************************\
 *                                                                     *
 * Constructors for particular instruction formats.  The offset is a   *
 * signed number and thus masked off to the right number of bits.  The *
 * other inputs are expected to be the right sizes.                    *
 *                                                                     *
 \***********************************************************************/
/*#define A_OFFSET 24*/
/*#define B_OFFSET 20*/
/*#define DST_OFFSET 0*/
/*#define OFFSET_OFFSET 0*/
/*#define X_OFFSET 24*/
/*#define Y_OFFSET 20*/
/*#define Z_OFFSET 0*/

static long construct_R(int opcode, int a, int b, int dst)
{
	return ((opcode << OP_OFFSET)
			| (a << Y_OFFSET)
			| (b << Z_OFFSET)
			| (dst << X_OFFSET));
}

static long construct_I(int opcode, int a, int b, int offset)
{
	return ((opcode << OP_OFFSET)
			| (a << Y_OFFSET)
			| (b << X_OFFSET)
			| ((offset & ((1 << OFFSET_BITS) - 1)) << OFFSET_OFFSET));
}

static long construct_J(int opcode, int a, int b)
{
	return ((opcode << OP_OFFSET)
			| (a << X_OFFSET)
			| (b << Y_OFFSET));
}

static long construct_OM(int opcode)
{
	return (opcode << OP_OFFSET);
}


static long construct_S(int opcode, int a, int b)
{
	/* Note: hardwired in -1 as offset (for push decrement) */
	return ((opcode << OP_OFFSET)
			| (a << X_OFFSET)
			| (b << Y_OFFSET)
			| ((-1 & ((1 << OFFSET_BITS) - 1)) << OFFSET_OFFSET));
}

/**********************************************************************\
 *                                                                    *
 * second_pass does the bulk of the work.  Re-read the input file and *
 * turn each line of assembly into a machine-code instruction.        *
 *                                                                    *
 \**********************************************************************/

static int second_pass(FILE * input, FILE * output)
{
	int pc = 0;                 /* program counter for symbol values */
	char *line;                 /* whole input line */
	char **tokens;              /* ARGV-style split of line */
	char *mnem;                 /* handy pointer to mnemonic */
	instruction_t *instruction; /* from the instruction table */
	unsigned int machinecode;   /* translated instruction */

	sourcelinenumber = 0;
	nerrors = 0;
	while ((line = getLine(input)) != NULL) {
		tokens = lexer(line);   /* convert line to tokens */
		sourcelinenumber++;     /* update line number for errors */

		/*
		 * blank line -- ignore it
		 */
		if (tokens[0] == NULL) {
			continue;
		}
		/*
		 * first token is a delimiter.  We already complained about this
		 * in the first pass to ignore this line also
		 */
		if (is_delimiter(tokens[0]))
			continue;

		/*
		 * if the second token is a ':', consider it a label
		 * and ignore it.
		 */
		if (tokens[1] == colon) {
			tokens++;           /* advance pointer to skip the label */
			tokens++;           /* advance pointer to skip the colon */
		}

		/*
		 * no opcode on this line, ignore the whole line
		 */
		mnem = tokens[0];       /* handy */
		if (mnem == NULL)
			continue;

		/*
		 * delimiter at this point is another syntax error
		 */
		if (is_delimiter(tokens[0])) {
			fprintf(stderr,
					"%s:%d: bad delimiter '%s' where mnemonic should be\n",
					sourcefilename, sourcelinenumber, tokens[0]);
			nerrors++;
			continue;
		}

		/*
		 * try to translate the opcode mnemonic.  If it fails,
		 * print a message and skip the rest of the line
		 */
		instruction = lookup_instruction(mnem);
		if (instruction == NULL) {
			fprintf(stderr,
					"%s:%d: bad opcode '%s'\n",
					sourcefilename, sourcelinenumber, mnem);
			nerrors++;
			machinecode = 0;
		} else {
			/*
			 * now the good part: switch on the instruction format
			 * and perform a consistency check for each format.
			 */
			switch (instruction->format) {
				/*
				 * R-type is three register operands
				 */
				case R:
					{
						int a, b, dst;

						if (!check_length(&tokens[1], 5, mnem))
							break;
						dst = token_to_regno(tokens[1], mnem);
						check_delimiter(tokens[2], comma, mnem);
						a = token_to_regno(tokens[3], mnem);
						check_delimiter(tokens[4], comma, mnem);
						b = token_to_regno(tokens[5], mnem);

						machinecode = construct_R(instruction->opcode, a, b, dst);
						break;
					}

					/*
					 * I-type is two registers plus an "offset":  LW R1, 4(R2)
					 * The offset argument is either a decimal/hex number or a symbol.
					 * The offset argument is treated as absolute by LW/SW
					 */
				case I:
					{
						int a, b, offset;

						if (!check_length(&tokens[1], 6, mnem))
							break;
						b = token_to_regno(tokens[1], mnem);
						check_delimiter(tokens[2], comma, mnem);
						offset = token_to_number(tokens[3], mnem);
						check_offset(offset, mnem);
						check_delimiter(tokens[4], lparen, mnem);
						a = token_to_regno(tokens[5], mnem);
						check_delimiter(tokens[6], rparen, mnem);

						machinecode = construct_I(instruction->opcode, a, b, offset);
						break;
					}

					/*
					 * II-type has two registers and offset:  BEQ R1, R2, dst
					 * The offset argument is either a decimal/hex number or a symbol.
					 * The offset argument is treated as absolute for ADDI
					 * and relative for BEQ
					 */
				case II:
					{
						int a, b, offset;

						if (!check_length(&tokens[1], 5, mnem))
							break;
						b = token_to_regno(tokens[1], mnem);
						check_delimiter(tokens[2], comma, mnem);
						a = token_to_regno(tokens[3], mnem);
						check_delimiter(tokens[4], comma, mnem);
						offset = token_to_number(tokens[5], mnem);

						if (!strcmp(mnem, "beq"))
							offset = offset - (pc + 1);

						check_offset(offset, mnem);
						machinecode = construct_I(instruction->opcode, a, b, offset);
						break;
					}

					/*
					 * J-type has two register operands
					 */
				case J:
					{
						int a, b;

						if (!check_length(&tokens[1], 3, mnem))
							break;
						a = token_to_regno(tokens[1], mnem);
						check_delimiter(tokens[2], comma, mnem);
						b = token_to_regno(tokens[3], mnem);

						machinecode = construct_J(instruction->opcode, a, b);
						break;
					}

					/*
					 * Both O-type and M-type have no operands
					 */
				case O:
				case M:
					{
						if (!check_length(&tokens[1], 0, mnem))
							break;

						machinecode = construct_OM(instruction->opcode);
						break;
					}

					/*
					 * P-type (a pseudo-op) has one operand.
					 */
				case P:
					{
						/*HACK la is just addi Rx <- $zero+OFFSET*/
						if (!strcmp(mnem, "la") && check_length(&tokens[1], 3, mnem)){
							int a, b, offset = 0;
							b = token_to_regno(tokens[1], mnem);
							check_delimiter(tokens[2], comma, mnem);
							a = 0;
							offset = token_to_number(tokens[3], mnem);

							check_offset(offset, mnem);
							machinecode = construct_I(OP_ADDI, a, b, offset);
							break;
						}

						/*case for .word*/
						if (!check_length(&tokens[1], 1, mnem))
							break;
						machinecode = token_to_number(tokens[1], mnem);

						break;
					}

					/*
					 * S-type has two register operands
					 */
				case S:
					{
						int a, b;

						if (!check_length(&tokens[1], 3, mnem))
							break;
						a = token_to_regno(tokens[1], mnem);
						check_delimiter(tokens[2], comma, mnem);
						b = token_to_regno(tokens[3], mnem);

						machinecode = construct_S(instruction->opcode, a, b);
						break;
					}

				default:
					assert(0);
			}                   /* [end of switch on the instruction type] */
		}                       /* [end of if/else known instruction] */

		/*
		 * spit out the translated instruction as a hex number.
		 */
		fprintf(output, "%08X ", machinecode);
		pc++;
	}                           /* [end of the big loop on input lines] */

	fprintf(stderr, "%d errors on the second pass\n", nerrors);
	return (nerrors == 0);      /* 1 on success, 0 on failure */
}                               /* [end of second_pass()] */

/**********************************\
 *                                *
 * construct the output filename. *
 *                                *
 \**********************************/

static void construct_output_filename(char *outname, char *inname)
{
	char *ptr;

	strcpy(outname, inname);
	ptr = strrchr(outname, '.');
	if (ptr)
		strcpy(ptr, OUTEXTENSION);
	else
		strcat(outname, OUTEXTENSION);
}

/**********************************************************************\
 *                                                                    *
 * Main program: run the two passes!  The input has to be from a file *
 * because we can't do a "rewind" on stdin.  Output is to stdout.     *
 *                                                                    *
 \**********************************************************************/

int main(int argc, char *argv[])
{
	char outfilename[MAXLINELEN];
	FILE *input, *output;

	/*
	 * read the filename argument and open it.
	 */
	if (argc < 2) {
		fprintf(stderr, "Usage: assemble <filename>\n");
		fprintf(stderr, "  Reads LC-S98 assembly language from the named\n");
		fprintf(stderr,
				"  file.  Writes the machine code as a sequence of\n");
		fprintf(stderr, "  hexadecimal numbers to stdout.\n");
		return (1);
	}
	sourcefilename = argv[1];
	input = fopen(sourcefilename, "r");
	if (input == NULL) {
		fprintf(stderr, "Unable to open '%s' for read\n", sourcefilename);
		return (1);
	}

	/*
	 * run the first pass.
	 */
	if (!first_pass(input)) {
		fprintf(stderr, "Exiting due to errors in the first pass\n");
		fclose(input);
		return (1);
	}

	/*
	 * rewind the input;
	 */
	rewind(input);

	/*
	 * construct the output file name and open that.
	 */
	construct_output_filename(outfilename, sourcefilename);
	output = fopen(outfilename, "w");
	if (output == NULL) {
		fprintf(stderr, "Unable to open '%s' for write\n", outfilename);
		fclose(input);
		return (1);
	}

	/*
	 * run the second pass.
	 */
	if (!second_pass(input, output)) {
		fprintf(stderr, "Exiting due to errors in the second pass\n");
		fclose(input);
		return (1);
	}

	/*
	 * success!
	 */
	fprintf(stderr, "Success!\n");
	fclose(input);
	fclose(output);
	return (0);
}
