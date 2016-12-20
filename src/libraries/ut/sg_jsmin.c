/* jsmin.c
   2011-01-22

Copyright (c) 2002 Douglas Crockford  (www.crockford.com)

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

The Software shall be used for Good, not Evil.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include <sg.h>

// Veracity mods - in/out via strings, state in this structure instead of global
typedef struct
{
	SG_int32 theA;
	SG_int32 theB;
	SG_int32 theLookahead;
	SG_uint32 sofar;
	SG_uint32 len;
	SG_int32 *input;
	SG_int32 *output;
	SG_uint32 outlen;
} sg_jsmin_state;




/* isAlphanum -- return true if the character is a letter, digit, underscore,
        dollar sign, or non-ASCII character.
*/

static int
isAlphanum(int c)
{
    return ((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') ||
        (c >= 'A' && c <= 'Z') || c == '_' || c == '$' || c == '\\' ||
        c > 126);
}


/* get -- return the next character from stdin. Watch out for lookahead. If
        the character is a control character, translate it to a space or
        linefeed.
*/

static int
get(SG_context *pCtx, sg_jsmin_state *state)
{
    int c = state->theLookahead;

	SG_UNUSED(pCtx);


    state->theLookahead = EOF;
    if (c == EOF) {
		if (state->sofar >= state->len)
			c = EOF;
		else
		{
			c = (int)state->input[state->sofar];
			++state->sofar;
		}
    }
    if (c >= ' ' || c == '\n' || c == EOF) {
        return c;
    }
    if (c == '\r') {
        return '\n';
    }
    return ' ';
}


/* peek -- get the next character without getting it.
*/

static int
peek(SG_context *pCtx, sg_jsmin_state *state)
{
    state->theLookahead = get(pCtx, state);
    return state->theLookahead;
}


/* next -- get the next character, excluding comments. peek() is used to see
        if a '/' is followed by a '/' or '*'.
*/

static int
next(SG_context *pCtx, sg_jsmin_state *state)
{
    int c = get(pCtx, state);
    if  (c == '/') {
        switch (peek(pCtx, state)) {
        case '/':
            for (;;) {
                c = get(pCtx, state);
                if (c <= '\n') {
                    return c;
                }
            }
        case '*':
            get(pCtx, state);
            for (;;) {
                switch (get(pCtx, state)) {
                case '*':
                    if (peek(pCtx, state) == '/') {
                        get(pCtx, state);
                        return ' ';
                    }
                    break;
                case EOF:
					SG_ERR_THROW2( SG_ERR_JSMIN_ERROR, (pCtx, "%s", "Unterminated comment.")  );
                }
            }
        default:
            return c;
        }
    }
    return c;

fail:
	return(EOF);
}


static void
_sg_jsmin_putc(SG_context *pCtx, sg_jsmin_state *state, int c)
{
	SG_UNUSED( pCtx );

	SG_ASSERT( state->outlen <= (state->len + 1) );
	state->output[state->outlen] = c;
	state->outlen++;
	state->output[state->outlen] = 0;
}


/* action -- do something! What you do is determined by the argument:
        1   Output A. Copy B to A. Get the next B.
        2   Copy B to A. Get the next B. (Delete A).
        3   Get the next B. (Delete B).
   action treats a string as a single character. Wow!
   action recognizes a regular expression if it is preceded by ( or , or =.
*/

static void
action(SG_context *pCtx, sg_jsmin_state *state, int d)
{
    switch (d) {
    case 1:
        _sg_jsmin_putc(pCtx, state, state->theA);
    case 2:
        state->theA = state->theB;
        if (state->theA == '\'' || state->theA == '"') {
            for (;;) {
                _sg_jsmin_putc(pCtx, state, state->theA);
                state->theA = get(pCtx, state);
                if (state->theA == state->theB) {
                    break;
                }
                if (state->theA == '\\') {
                    _sg_jsmin_putc(pCtx, state, state->theA);
                    state->theA = get(pCtx, state);
                }
                if (state->theA == EOF) {
					SG_ERR_THROW2(  SG_ERR_JSMIN_ERROR, (pCtx, "%s", "Unterminated string literal")  );
                }
            }
        }
    case 3:
        state->theB = next(pCtx, state);
        if (state->theB == '/' && (state->theA == '(' || state->theA == ',' || state->theA == '=' ||
                            state->theA == ':' || state->theA == '[' || state->theA == '!' ||
                            state->theA == '&' || state->theA == '|' || state->theA == '?' ||
                            state->theA == '{' || state->theA == '}' || state->theA == ';' ||
                            state->theA == '\n')) {
            _sg_jsmin_putc(pCtx, state, state->theA);
            _sg_jsmin_putc(pCtx, state, state->theB);
            for (;;) {
                state->theA = get(pCtx, state);
                if (state->theA == '[') {
					for (;;) {
						_sg_jsmin_putc(pCtx, state, state->theA);
						state->theA = get(pCtx, state);
						if (state->theA == ']') {
							break;
						} 
						if (state->theA == '\\') {
							_sg_jsmin_putc(pCtx, state, state->theA);
							state->theA = get(pCtx, state);
						} 
						if (state->theA == EOF) {
							SG_ERR_THROW2(  SG_ERR_JSMIN_ERROR, (pCtx, "%s", "Unterminated set in Regular Expression literal")  );
						}
					}
				} else if (state->theA == '/') {
                    break;
                } else if (state->theA =='\\') {
                    _sg_jsmin_putc(pCtx, state, state->theA);
                    state->theA = get(pCtx, state);
                }
                if (state->theA == EOF) {
					SG_ERR_THROW2(  SG_ERR_JSMIN_ERROR, (pCtx, "%s", "unterminated Regular Expression literal")  );
                }
                _sg_jsmin_putc(pCtx, state, state->theA);
            }
            state->theB = next(pCtx, state);
        }
    }

fail:
	;
}


/* jsmin -- Copy the input to the output, deleting the characters which are
        insignificant to JavaScript. Comments will be removed. Tabs will be
        replaced with spaces. Carriage returns will be replaced with linefeeds.
        Most spaces and linefeeds will be removed.
*/

static void
jsmin(SG_context *pCtx, sg_jsmin_state *state)
{
    state->theA = '\n';
    action(pCtx, state, 3);
    while (state->theA != EOF) {
        switch (state->theA) {
        case ' ':
            if (isAlphanum(state->theB)) {
                action(pCtx, state, 1);
            } else {
                action(pCtx, state, 2);
            }
            break;
        case '\n':
            switch (state->theB) {
            case '{':
            case '[':
            case '(':
            case '+':
            case '-':
                action(pCtx, state, 1);
                break;
            case ' ':
                action(pCtx, state, 3);
                break;
            default:
                if (isAlphanum(state->theB)) {
                    action(pCtx, state, 1);
                } else {
                    action(pCtx, state, 2);
                }
            }
            break;
        default:
            switch (state->theB) {
            case ' ':
                if (isAlphanum(state->theA)) {
                    action(pCtx, state, 1);
                    break;
                }
                action(pCtx, state, 3);
                break;
            case '\n':
                switch (state->theA) {
                case '}':
                case ']':
                case ')':
                case '+':
                case '-':
                case '"':
                case '\'':
                    action(pCtx, state, 1);
                    break;
                default:
                    if (isAlphanum(state->theA)) {
                        action(pCtx, state, 1);
                    } else {
                        action(pCtx, state, 3);
                    }
                }
                break;
            default:
                action(pCtx, state, 1);
                break;
            }
        }
    }
}


void SG_jsmin(SG_context *pCtx, const char *input, SG_string *output)
{
	sg_jsmin_state st;
	SG_int32 *unicode = NULL;  // FREE THIS
	SG_int32 *uniresult = NULL;  // FREE THIS
	char *result = NULL; // FREE THIS
	SG_uint32 len;

	SG_NULLARGCHECK_RETURN(input);
	SG_NULLARGCHECK_RETURN(output);

	st.theA = st.theB = st.theLookahead = EOF;
	st.sofar = 0;

	SG_ERR_CHECK( SG_utf8__to_utf32__sz( pCtx, input, &unicode, &len ) );

	st.input = unicode;
	st.len = len;
	st.outlen = 0;

	SG_ERR_CHECK(  SG_alloc(pCtx, len + 2, sizeof(SG_int32), &uniresult)  );

	st.output = uniresult;

	jsmin(pCtx, &st);

	SG_ERR_CHECK(  SG_utf8__from_utf32(pCtx, uniresult, &result, &len)  );

	SG_ERR_CHECK(  SG_string__set__sz(pCtx, output, result)  );

fail:
	SG_NULLFREE(pCtx, result);
	SG_NULLFREE(pCtx, uniresult);
	SG_NULLFREE(pCtx, unicode);
}


/* main -- Output any command line arguments as comments
        and then minify the input.

extern int
main(int argc, char* argv[])
{
    int i;
    for (i = 1; i < argc; i += 1) {
        fprintf(stdout, "// %s\n", argv[i]);
    }
    jsmin();
    return 0;
}
*/
