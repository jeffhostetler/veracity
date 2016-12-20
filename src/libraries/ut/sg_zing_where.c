/*
Copyright 2010-2013 SourceGear, LLC

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#include <sg.h>

#include "sg_zing__private.h"

// TODO temporary hack:
#ifdef WINDOWS
//#pragma warning(disable:4100)
#pragma warning(disable:4706)
#endif

enum token
{
    TOK_UNKNOWN,
    TOK_IDENT,
    TOK_GT,
    TOK_LT,
    TOK_GE,
    TOK_LE,
    TOK_EQ,
    TOK_NE,
    TOK_LPAREN,
    TOK_RPAREN,
    TOK_AND,
    TOK_OR,
    TOK_INT,
    TOK_STRING,
    TOK_NULL,
    TOK_TRUE,
    TOK_FALSE,
    TOK_MATCH,
    TOK_END
};

struct sg_where_parser
{
    const char* psz_rectype;
    SG_uint32 next_token;
    const char* pcur;
    const char* token;
    SG_uint32 len_token;
    SG_varray* pva_top;
    SG_string* pstr;
};

#define CURCHAR (*(pw->pcur))
#define NEXTCHAR (pw->pcur++)

static void w_token(
        SG_context* pCtx,
        struct sg_where_parser* pw
        )
{
    while (
            (CURCHAR)
            && (
                (' ' == CURCHAR)
                || ('\t' == CURCHAR)
                || ('\n' == CURCHAR)
                || ('\r' == CURCHAR)
               )
          )
    {
        pw->pcur++;
    }
    pw->token = pw->pcur;
    if (0 == CURCHAR)
    {
        pw->next_token = TOK_END;
    }
    else if ('(' == CURCHAR)
    {
        NEXTCHAR;
        pw->next_token = TOK_LPAREN;
    }
    else if (')' == CURCHAR)
    {
        NEXTCHAR;
        pw->next_token = TOK_RPAREN;
    }
    else if ('>' == CURCHAR)
    {
        NEXTCHAR;
        if ('=' == CURCHAR)
        {
            NEXTCHAR;
            pw->next_token = TOK_GE;
        }
        else
        {
            pw->next_token = TOK_GT;
        }
    }
    else if ('<' == CURCHAR)
    {
        NEXTCHAR;
        if ('=' == CURCHAR)
        {
            NEXTCHAR;
            pw->next_token = TOK_LE;
        }
        else
        {
            pw->next_token = TOK_LT;
        }
    }
    else if ('=' == CURCHAR)
    {
        NEXTCHAR;
        if ('=' == CURCHAR)
        {
            NEXTCHAR;
            pw->next_token = TOK_EQ;
        }
        else
        {
#if 1
            // we accept either == or = for comparison
            pw->next_token = TOK_EQ;
#else
            SG_ERR_THROW2(  SG_ERR_ZING_WHERE_PARSE_ERROR,
                    (pCtx, "Error in == at %s", pw->pcur)
                    );
#endif
        }
    }
    else if ('#' == CURCHAR)
    {
        SG_bool bok = SG_FALSE;

        NEXTCHAR;
        if ('T' == CURCHAR)
        {
            NEXTCHAR;
            if ('R' == CURCHAR)
            {
                NEXTCHAR;
                if ('U' == CURCHAR)
                {
                    NEXTCHAR;
                    if ('E' == CURCHAR)
                    {
                        NEXTCHAR;
                        bok = SG_TRUE;
                        pw->next_token = TOK_TRUE;
                    }
                }
            }
            else
            {
                bok = SG_TRUE;
                pw->next_token = TOK_TRUE;
            }
        }
        else if ('F' == CURCHAR)
        {
            NEXTCHAR;
            if ('A' == CURCHAR)
            {
                NEXTCHAR;
                if ('L' == CURCHAR)
                {
                    NEXTCHAR;
                    if ('S' == CURCHAR)
                    {
                        NEXTCHAR;
                        if ('E' == CURCHAR)
                        {
                            NEXTCHAR;
                            bok = SG_TRUE;
                            pw->next_token = TOK_FALSE;
                        }
                    }
                }
            }
            else
            {
                bok = SG_TRUE;
                pw->next_token = TOK_FALSE;
            }
        }
        else if ('M' == CURCHAR)
        {
            NEXTCHAR;
            if ('A' == CURCHAR)
            {
                NEXTCHAR;
                if ('T' == CURCHAR)
                {
                    NEXTCHAR;
                    if ('C' == CURCHAR)
                    {
                        NEXTCHAR;
                        if ('H' == CURCHAR)
                        {
                            NEXTCHAR;
                            bok = SG_TRUE;
                            pw->next_token = TOK_MATCH;
                        }
                    }
                }
            }
            else
            {
                bok = SG_TRUE;
                pw->next_token = TOK_FALSE;
            }
        }
        else if ('N' == CURCHAR)
        {
            NEXTCHAR;
            if ('U' == CURCHAR)
            {
                NEXTCHAR;
                if ('L' == CURCHAR)
                {
                    NEXTCHAR;
                    if ('L' == CURCHAR)
                    {
                        NEXTCHAR;
                        bok = SG_TRUE;
                        pw->next_token = TOK_NULL;
                    }
                }
            }
            else
            {
                bok = SG_TRUE;
                pw->next_token = TOK_FALSE;
            }
        }
        if (!bok)
        {
            SG_ERR_THROW2(  SG_ERR_ZING_SORT_PARSE_ERROR,
                    (pCtx, "Error at %s", pw->pcur)
                    );
        }
    }
    else if ('!' == CURCHAR)
    {
        NEXTCHAR;
        if ('=' == CURCHAR)
        {
            NEXTCHAR;
            pw->next_token = TOK_NE;
        }
        else
        {
            SG_ERR_THROW2(  SG_ERR_ZING_WHERE_PARSE_ERROR,
                    (pCtx, "Error in != at %s", pw->pcur)
                    );
        }
    }
    else if ('&' == CURCHAR)
    {
        NEXTCHAR;
        if ('&' == CURCHAR)
        {
            NEXTCHAR;
            pw->next_token = TOK_AND;
        }
        else
        {
            SG_ERR_THROW2(  SG_ERR_ZING_WHERE_PARSE_ERROR,
                    (pCtx, "Error in && at %s", pw->pcur)
                    );
        }
    }
    else if ('|' == CURCHAR)
    {
        NEXTCHAR;
        if ('|' == CURCHAR)
        {
            NEXTCHAR;
            pw->next_token = TOK_OR;
        }
        else
        {
            SG_ERR_THROW2(  SG_ERR_ZING_WHERE_PARSE_ERROR,
                    (pCtx, "Error in || at %s", pw->pcur)
                    );
        }
    }
    else if ('\'' == CURCHAR)
    {
        NEXTCHAR;
        SG_ERR_CHECK(  SG_string__clear(pCtx, pw->pstr)  );
        while (CURCHAR)
        {
            SG_uint32 count = 0;
            while(CURCHAR=='\'')
            {
                ++count;
                NEXTCHAR;
            }
            while(count>1)
            {
                count -= 2;
                SG_ERR_CHECK(  SG_string__append__sz(pCtx, pw->pstr, "\'")  );
            }
            if(count == 1)
                break;
            
            while (
                    CURCHAR
                    && (CURCHAR != '\'')
                  )
            {
                SG_ERR_CHECK(  SG_string__append__buf_len(pCtx, pw->pstr, (SG_byte*) pw->pcur, 1)  );
                NEXTCHAR;
            }
        }
        pw->next_token = TOK_STRING;
    }
    else if (
            ( (CURCHAR >= '0') && (CURCHAR <= '9') )
            || ('-' == CURCHAR)
            )
    {
        NEXTCHAR;
        while ((CURCHAR >= '0') && (CURCHAR <= '9') )
        {
            NEXTCHAR;
        }
        pw->next_token = TOK_INT;
    }
    else if (
            ( (CURCHAR >= 'a') && (CURCHAR <= 'z') )
            || ( (CURCHAR >= 'A') && (CURCHAR <= 'Z') )
            || ('_' == CURCHAR)
            )
    {
        NEXTCHAR;
        while (
            ( (CURCHAR >= 'a') && (CURCHAR <= 'z') )
            ||( (CURCHAR >= 'A') && (CURCHAR <= 'Z') )
            ||( (CURCHAR >= '0') && (CURCHAR <= '9') )
            || ('_' == CURCHAR)
            )
        {
            NEXTCHAR;
        }
        pw->next_token = TOK_IDENT;
    }
    else
    {
        SG_ERR_THROW2(  SG_ERR_ZING_WHERE_PARSE_ERROR,
                (pCtx, "Error at %s", pw->pcur)
                );
    }

    pw->len_token = (SG_uint32)(pw->pcur - pw->token);

fail:
    return;
}

#define EXPECT(tok) if (tok != pw->next_token) { SG_ERR_THROW2(  SG_ERR_ZING_WHERE_PARSE_ERROR, (pCtx, "Error at %s", pw->pcur) ); } SG_ERR_CHECK(  w_token(pCtx, pw)  )

static void w_expr(
        SG_context* pCtx,
        struct sg_where_parser* pw,
        SG_varray** pp
        );

static void w_predicate(
        SG_context* pCtx,
        struct sg_where_parser* pw,
        SG_varray** pp
        )
{
    SG_varray* pva = NULL;
    char buf_name[SG_ZING_MAX_FIELD_NAME_LENGTH + 1];
    SG_uint32 op = 0;

    if (pw->pva_top)
    {
        SG_ERR_CHECK(  SG_varray__alloc__shared(pCtx, &pva, 3, pw->pva_top)  );
    }
    else
    {
        SG_ERR_CHECK(  SG_VARRAY__ALLOC(pCtx, &pva)  );
    }

    if (TOK_IDENT != pw->next_token)
    {
        SG_ERR_THROW2(  SG_ERR_ZING_WHERE_PARSE_ERROR,
                (pCtx, "Error at %s", pw->pcur)
                );
    }
    if (pw->len_token > SG_ZING_MAX_FIELD_NAME_LENGTH)
    {
        SG_ERR_THROW2(  SG_ERR_ZING_WHERE_PARSE_ERROR,
                (pCtx, "Name too long to be a field name: %s", pw->pcur)
                );
    }

    memcpy(buf_name, pw->token, pw->len_token);
    buf_name[pw->len_token] = 0;

    SG_ERR_CHECK(  SG_varray__append__string__buflen(pCtx, pva, pw->token, pw->len_token)  );
    SG_ERR_CHECK(  w_token(pCtx, pw)  );
    switch (pw->next_token)
    {
        case TOK_MATCH:
            op = pw->next_token;
            SG_ERR_CHECK(  SG_varray__append__string__sz(pCtx, pva, "match")  );
            SG_ERR_CHECK(  w_token(pCtx, pw)  );
            break;

        case TOK_EQ:
            op = pw->next_token;
            SG_ERR_CHECK(  SG_varray__append__string__sz(pCtx, pva, "==")  );
            SG_ERR_CHECK(  w_token(pCtx, pw)  );
            break;

        case TOK_GT:
        case TOK_LT:
        case TOK_GE:
        case TOK_LE:
        case TOK_NE:
            op = pw->next_token;
            SG_ERR_CHECK(  SG_varray__append__string__buflen(pCtx, pva, pw->token, pw->len_token)  );
            SG_ERR_CHECK(  w_token(pCtx, pw)  );
            break;

        default:
            SG_ERR_THROW2(  SG_ERR_ZING_WHERE_PARSE_ERROR,
                    (pCtx, "Error at %s", pw->pcur)
                    );
            break;
    }
    switch (pw->next_token)
    {
        case TOK_NULL:
            if (
                    (op != TOK_EQ)
                    && (op != TOK_NE)
               )
            {
                SG_ERR_THROW2(  SG_ERR_ZING_WHERE_PARSE_ERROR,
                        (pCtx, "no < > <= >= allowed with #NULL")
                        );
            }
            SG_ERR_CHECK(  SG_varray__append__null(pCtx, pva)  );
            SG_ERR_CHECK(  w_token(pCtx, pw)  );
            break;

        case TOK_TRUE:
        case TOK_FALSE:
            if (
                    (op != TOK_EQ)
                    && (op != TOK_NE)
               )
            {
                SG_ERR_THROW2(  SG_ERR_ZING_WHERE_PARSE_ERROR,
                        (pCtx, "%s a bool field, no < > <= >= allowed", buf_name)
                        );
            }
            if (TOK_TRUE == pw->next_token)
            {
                SG_ERR_CHECK(  SG_varray__append__string__sz(pCtx, pva, "1")  );
            }
            else
            {
                SG_ERR_CHECK(  SG_varray__append__string__sz(pCtx, pva, "0")  );
            }
            SG_ERR_CHECK(  w_token(pCtx, pw)  );
            break;

        case TOK_INT:
            SG_ERR_CHECK(  SG_varray__append__string__buflen(pCtx, pva, pw->token, pw->len_token)  );
            SG_ERR_CHECK(  w_token(pCtx, pw)  );
            break;

        case TOK_STRING:
            SG_ERR_CHECK(  SG_varray__append__string__sz(pCtx, pva, SG_string__sz(pw->pstr))  );
            SG_ERR_CHECK(  w_token(pCtx, pw)  );
            break;

        default:
            SG_ERR_THROW2(  SG_ERR_ZING_WHERE_PARSE_ERROR,
                    (pCtx, "Error at %s", pw->pcur)
                    );
            break;
    }

    *pp = pva;
    pva = NULL;

fail:
    SG_VARRAY_NULLFREE(pCtx, pva);
}

static void w_boolean(
        SG_context* pCtx,
        struct sg_where_parser* pw,
        SG_varray** pp
        )
{
    SG_varray* pva_left = NULL;
    SG_varray* pva_right = NULL;
    SG_varray* pva = NULL;

    if (pw->pva_top)
    {
        SG_ERR_CHECK(  SG_varray__alloc__shared(pCtx, &pva, 3, pw->pva_top)  );
    }
    else
    {
        SG_ERR_CHECK(  SG_VARRAY__ALLOC(pCtx, &pva)  );
    }

    EXPECT(TOK_LPAREN);
    SG_ERR_CHECK(  w_expr(pCtx, pw, &pva_left)  );
    SG_ERR_CHECK(  SG_varray__append__varray(pCtx, pva, &pva_left)  );
    EXPECT(TOK_RPAREN);
    switch (pw->next_token)
    {
        case TOK_AND:
        case TOK_OR:
            SG_ERR_CHECK(  SG_varray__append__string__buflen(pCtx, pva, pw->token, pw->len_token)  );
            SG_ERR_CHECK(  w_token(pCtx, pw)  );
            break;
        default:
            SG_ERR_THROW2(  SG_ERR_ZING_WHERE_PARSE_ERROR,
                    (pCtx, "Error at %s", pw->pcur)
                    );
            break;
    }
    EXPECT(TOK_LPAREN);
    SG_ERR_CHECK(  w_expr(pCtx, pw, &pva_right)  );
    SG_ERR_CHECK(  SG_varray__append__varray(pCtx, pva, &pva_right)  );
    EXPECT(TOK_RPAREN);

    *pp = pva;
    pva = NULL;

fail:
    SG_VARRAY_NULLFREE(pCtx, pva_left);
    SG_VARRAY_NULLFREE(pCtx, pva_right);
    SG_VARRAY_NULLFREE(pCtx, pva);
}

static void w_expr(
        SG_context* pCtx,
        struct sg_where_parser* pw,
        SG_varray** pp
        )
{
    if (TOK_IDENT == pw->next_token)
    {
        SG_ERR_CHECK(  w_predicate(pCtx, pw, pp)  );
    }
    else if (TOK_LPAREN == pw->next_token)
    {
        SG_ERR_CHECK(  w_boolean(pCtx, pw, pp)  );
    }
    else
    {
        SG_ERR_THROW2(  SG_ERR_ZING_WHERE_PARSE_ERROR,
                (pCtx, "Error at %s", pw->pcur)
                );
    }

fail:
    return;
}

static void sg_where__parse(
        SG_context* pCtx,
        struct sg_where_parser* pw,
        SG_varray** pp
        )
{
    SG_ERR_CHECK(  w_token(pCtx, pw)  );

    SG_ERR_CHECK(  w_expr(pCtx, pw, pp)  );

    if (TOK_END != pw->next_token)
    {
        SG_ERR_THROW2(  SG_ERR_ZING_WHERE_PARSE_ERROR,
                (pCtx, "Error at %s", pw->token)
                );
    }


fail:
    return;
}

void sg_zing__query__parse_where(
        SG_context* pCtx,
        const char* psz_rectype,
        const char* psz_where,
        SG_varray** pp
        )
{
    struct sg_where_parser w;
    SG_varray* pva_rectype = NULL;
    SG_varray* pva_where = NULL;

    memset(&w, 0, sizeof(w));

    if (psz_where)
    {
        w.psz_rectype = psz_rectype;
        w.pcur = psz_where;
        SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &w.pstr)  );

        // parse
        SG_ERR_CHECK(  sg_where__parse(pCtx, &w, &pva_where)  );

        *pp = pva_where;
        pva_where = NULL;
    }
    else
    {
        *pp = NULL;
    }

    // fall thru

fail:
    SG_STRING_NULLFREE(pCtx, w.pstr);
    SG_VARRAY_NULLFREE(pCtx, pva_rectype);
    SG_VARRAY_NULLFREE(pCtx, pva_where);
    SG_VARRAY_NULLFREE(pCtx, w.pva_top);
}

