#include "PL0_WordParser.h"


namespace PL0
{
    bool WordParser::Parse(_In_ const std::string& content)
    {
        return Parse(content.c_str());
    }

    bool WordParser::Parse(_In_ const char* content)
    {
        m_pCurrContent = content;
        m_Symbols.clear();
        m_ErrorInfos.clear();
        m_CurrCoord = { 1,1 };

        ErrorInfo errorInfo{};
        Symbol symbol{};

        while (*m_pCurrContent)
        {
            symbol = GetNextSymbol(&errorInfo);
            //注释部分忽略,否则将词语放入符号集
            if (symbol.symbolType != ST_Comment)
            {
                m_Symbols.push_back(symbol);
            }

            //如有错误码则记录错误信息
            if (errorInfo.errorCode != 0)
            {
                m_ErrorInfos.push_back(errorInfo);
            }
        }

        //若无错误信息则意味着分析成功
        return m_ErrorInfos.empty();
    }

    const std::vector<Symbol>& WordParser::GetSymbols() const
    {
        return m_Symbols;
    }

    const std::vector<ErrorInfo>& WordParser::GetErrorInfos() const
    {
        return m_ErrorInfos;
    }

    char WordParser::GetNextChar()
    {
        char ch = *m_pCurrContent;
        //已经到达代码尾部
        if (ch == '\0')
        {
            return '\0';
        }
        //已经达到当前行末
        if (ch == '\n')
        {
            ++m_CurrCoord.row;
            m_CurrCoord.col = 1;
        }
        else
        {
            ++m_CurrCoord.col;
        }
        ++m_pCurrContent;
        return ch;
    }

    void WordParser::IgnoreNextChar()
    {
        char ch = GetNextChar();
    }

    char WordParser::PeekNextChar()
    {
        return *m_pCurrContent;
    }

    Symbol WordParser::GetNextSymbol(_Out_ ErrorInfo* perrorInfo)
    {
        Symbol symbol;
        symbol.symbolType = ST_Null;

        ErrorInfo errorInfo{};
        errorInfo.beg = errorInfo.end = m_CurrCoord;
        errorInfo.errorCode = 0;

        //抓取下一个字符
        char ch;

        //清理换行符，制表符，空格
        while (isspace(ch = PeekNextChar()))
        {
            IgnoreNextChar();
        }

        //记录起始内容和位置
        const char* startContent = m_pCurrContent;
        symbol.beg = m_CurrCoord;

        ch = GetNextChar();
#pragma region 关键字或标识符
        //符号类型可能是关键字或标识符 [keyword][indeitfier]
        if (isalpha(ch) || ch == '_')
        {
            ch = PeekNextChar();
            while (isalpha(ch) || isdigit(ch) || ch == '_')
            {
                IgnoreNextChar();
                ch = PeekNextChar();
            }

            //判断是否为关键字
            std::string word(startContent, m_pCurrContent);
            for (size_t i = 0; i < g_KeywordCount; ++i)
            {
                if (word == g_KeyWords[i])
                {
                    symbol.symbolType = static_cast<SymbolType>(g_KeywordOffset + i);
                    break;
                }
            }

            //不是关键字的话则为标识符
            if (symbol.symbolType == ST_Null)
            {
                if (symbol.word.size() > g_MaxIdentifierLength)
                {
                    errorInfo.errorCode = 12;                       //[Error12] Identifier length exceed
                }
                else
                {
                    symbol.symbolType = ST_Identifier;
                }
            }
        }
#pragma endregion
#pragma region 整型或浮点型
        else if (isdigit(ch))
        {
            //符号型别为整型
            ch = PeekNextChar();
            while (isdigit(ch))
            {
                IgnoreNextChar();
                ch = PeekNextChar();
            }

            symbol.symbolType = ST_Integer;

            //符号型别为浮点型
            if (ch == '.')
            {
                symbol.symbolType = ST_Float;

                do
                {
                    IgnoreNextChar();
                    ch = PeekNextChar();
                } while (isdigit(ch));

                //科学计数法
                if (ch == 'e')
                {
                    IgnoreNextChar();
                    ch = PeekNextChar();
                    if (ch == '+' || ch == '-')
                    {
                        IgnoreNextChar();
                        ch = PeekNextChar();
                    }

                    int count = 0;
                    do
                    {
                        IgnoreNextChar();
                        ch = PeekNextChar();
                        ++count;
                    } while (isdigit(ch));

                    if (count == 0)
                    {
                        errorInfo.errorCode = 2;            //[Error2] Invaid value
                    }
                    
                }
            }

            if (symbol.symbolType == ST_Integer && m_pCurrContent - startContent > g_MaxIntegerLength)
            {
                errorInfo.errorCode = 1;                //[Error01] Integer length exceed
            }
            else if (isalpha(ch) || ch == '_')
            {
                errorInfo.errorCode = 2;                //[Error02] Invalid value
                //处理剩下的衔接部分
                do
                {
                    IgnoreNextChar();
                    ch = PeekNextChar();
                } while (isdigit(ch) || isalpha(ch) || ch == '_');

                symbol.symbolType = ST_Null;

            }
        }
#pragma endregion

#pragma region 字符型
        //符号类型是字符型 'a'
        else if (ch == '\'')
        {
            const char* beginCharStr = m_pCurrContent;
            const char* endCharStr = nullptr;

            //尝试分析字符
            TryParseChar(beginCharStr, &endCharStr);

            while (endCharStr - m_pCurrContent > 0)
            {
                IgnoreNextChar();
            }
            ch = PeekNextChar();

            if (ch != '\'')
            {
                //抓取下一个字符尾，行尾或结束
                if (ch != '\'' && ch != '\n' && ch != '\0')
                {
                    ch = PeekNextChar();
                    while (ch != '\'' && ch != '\n' && ch != '\0')
                    {
                        IgnoreNextChar();
                        ch = PeekNextChar();
                    }
                }

                errorInfo.errorCode = 6;        //[Error06] '\''Expected
            }

            //扫到下一个 '时需要避开
            if (ch == '\'')
            {
                //字符内没有内容 ''
                if (m_pCurrContent == beginCharStr)
                {
                    errorInfo.errorCode = 7;            //[Error07] Charcter expected
                }
                //塞入过多字符
                else if (m_pCurrContent > endCharStr)
                {
                    errorInfo.errorCode = 11;           //[Error11] Too much Character
                }
                IgnoreNextChar();
            }
        }
#pragma endregion
#pragma region 字符串型
        //符号型别时字符串型 "abc"
        else if (ch == '\"')
        {
            const char* beginCharStr = m_pCurrContent;
            const char* endCharStr = beginCharStr + 1;

            //尝试一直分析字符
            while (*endCharStr != '\"' && *endCharStr != '\n' && *endCharStr != '\0')
            {
                TryParseChar(m_pCurrContent, &endCharStr);

                while (endCharStr - m_pCurrContent > 0)
                {
                    IgnoreNextChar();
                }
            }

            ch = PeekNextChar();
            if (ch != '\"')
            {
                errorInfo.errorCode = 8;            //[Error08] '\"' Expected
            }
            else
            {
                IgnoreNextChar();
            }
        }
#pragma endregion
#pragma region 赋值
        else if (ch == ':')
        {
            ch = PeekNextChar();
            if (ch == '=')
            {
                IgnoreNextChar();
                symbol.symbolType = ST_Assign;
            }
            else
            {
                errorInfo.errorCode = 3;            //[Error03] Expect '=' after ':'

            }
        }
#pragma endregion

#pragma region 比较运算符与逻辑运算符
        else if (ch == '=')
        {
            symbol.symbolType = ST_Equal;
        }
        else if (ch == '>')
        {
            ch = PeekNextChar();
            //符号类型是大于等于 [>=]
            if (ch == '=')
            {
                IgnoreNextChar();
                symbol.symbolType = ST_GreaterEqual;
            }
            //符号类型是大于等于 [>]
            else
            {
                symbol.symbolType = ST_Greater;
            }
        }
        else if (ch == '<')
        {
            ch = PeekNextChar();
            //符号类型是大于等于 [<=]
            if (ch == '=')
            {
                IgnoreNextChar();
                symbol.symbolType = ST_LessEqual;
            }
            //符号类型是大于等于 [<]
            else
            {
                symbol.symbolType = ST_Less;
            }
        }
        else if (ch == '!')
        {
            ch = PeekNextChar();
            //符号类型是大于等于 [!=]
            if (ch == '=')
            {
                IgnoreNextChar();
                symbol.symbolType = ST_NotEqual;
            }
            //符号类型是大于等于 [<]
            else
            {
                symbol.symbolType = ST_LogicalNot;
            }
        }
#pragma endregion
#pragma region  逻辑运算符
        else if (ch == '|')
        {
            ch = PeekNextChar();
            if (ch == '|')
            {
                IgnoreNextChar();
                symbol.symbolType = ST_LogicalOr;
            }
            else
            {
                errorInfo.errorCode = 9;            //[Error09] Extra'|'Expected
            }
        }
        else if (ch == '&')
        {
            ch = PeekNextChar();
            if (ch == '&')
            {
                IgnoreNextChar();
                symbol.symbolType = ST_LogicalAnd;
            }
            else
            {
                errorInfo.errorCode = 10;            //[Error10] Extra'&'Expected
            }
        }
#pragma endregion
#pragma region 一般运算符与注释
        else if (ch == '+')
        {
            ch = PeekNextChar();
            //符号类型是加后赋值[+=]
            if (ch == '=')
            {
                IgnoreNextChar();
                symbol.symbolType = ST_PlusAndAssign;
            }
            //符号类型是自增++
            else if (ch == '+')
            {
                IgnoreNextChar();
                symbol.symbolType = ST_SelfAdd;
            }
            //符号类型是加法[+]
            else
            {
                symbol.symbolType = ST_Plus;
            }
        }
        else if (ch == '-')
        {
            ch = PeekNextChar();
            //符号类型是减后赋值[-=]
            if (ch == '=')
            {
                IgnoreNextChar();
                symbol.symbolType = ST_MinusAndAssign;
            }
            //符号类型是自增++
            else if (ch == '-')
            {
                IgnoreNextChar();
                symbol.symbolType = ST_SelfSub;
            }
            //符号类型是减法[-]
            else
            {
                symbol.symbolType = ST_Minus;
            }
        }
        else if (ch == '*')
        {
            ch = PeekNextChar();
            //符号类型是乘后赋值[*=]
            if (ch == '=')
            {
                IgnoreNextChar();
                symbol.symbolType = ST_MultiplyAndAssign;
            }
            //符号类型是乘法[*]
            else
            {
                symbol.symbolType = ST_Multiply;
            }
        }
        else if (ch == '/')
        {
            ch = PeekNextChar();
            //符号类型是除后赋值[/=]
            if (ch == '=')
            {
                IgnoreNextChar();
                symbol.symbolType = ST_DivideAndAssign;
            }
            //符号类型是注释[//]
            else if (ch == '/')
            {
                IgnoreNextChar();
                ch = PeekNextChar();
                while (ch && ch != '\n')
                {
                    // \后接换行符注释下一行
                    if (ch == '\\')
                    {
                        IgnoreNextChar();
                    }
                    IgnoreNextChar();
                    ch = PeekNextChar();
                }
                symbol.symbolType = ST_Comment;
            }
            //符号类型是注释[/*.....*/]
            else if (ch == '*')
            {
                IgnoreNextChar();
                ch = PeekNextChar();
                // 寻找*/
                while (ch)
                {
                    IgnoreNextChar();
                    if (ch == '*' && (ch = PeekNextChar() == '/'))
                    {
                        IgnoreNextChar();
                        break;
                    }
                    ch = PeekNextChar();
                }
                //如果没有找到，则报错
                if (!ch)
                {
                    errorInfo.errorCode = 4;            //[Error004] Missing*/
                }
                symbol.symbolType = ST_Comment;
            }
            //符号类型是除法[/]
            else
            {
                symbol.symbolType = ST_Divide;
            }
        }
        else if (ch == '%')
        {
            ch = PeekNextChar();
            //符号类型是取余后赋值[%=]
            if (ch == '=')
            {
                IgnoreNextChar();
                symbol.symbolType = ST_ModAndAssign;
            }
            //符号类型是取余[%]
            else
            {
                symbol.symbolType = ST_Mod;
            }
        }
#pragma endregion
#pragma region 括号
        else if (ch == '(')
        {
            symbol.symbolType = ST_LeftParen;
        }
        else if (ch == ')')
        {
            symbol.symbolType = ST_RightParen;
        }
        else if (ch == '[')
        {
            symbol.symbolType = ST_LeftBracket;
        }
        else if (ch == ']')
        {
            symbol.symbolType = ST_RightBracket;
        }
        else if (ch == '{')
        {
            symbol.symbolType = ST_LeftBrace;
        }
        else if (ch == '}')
        {
            symbol.symbolType = ST_RightBrace;
        }
#pragma endregion
#pragma region 分隔符
        else if (ch == ',')
        {
            symbol.symbolType = ST_Comma;
        }
        else if (ch == '.')
        {
            symbol.symbolType = ST_Period;
        }
        else if (ch == ';')
        {
            symbol.symbolType = ST_SemiColon;
        }
        else
        {
            errorInfo.errorCode = 5;                //[Error05] Unknown Character
        }
#pragma endregion

        symbol.end = m_CurrCoord;


        if (perrorInfo)
        {
            errorInfo.end = m_CurrCoord;
            *perrorInfo = errorInfo;
        }

        symbol.word = std::string(startContent, m_pCurrContent);

        //清理换行符，制表符，空格
        while (ch && isspace(PeekNextChar()))
        {
            ch = GetNextChar();
        }

        return symbol;
    }


    void WordParser::TryParseChar(_In_ const char* pcontent, _Out_ const char** pEndCharPos)
    {
        if (!pcontent[0] || pcontent[0] == '\'')
        {
            *pEndCharPos = pcontent;
        }
        //转义字符
        else if (pcontent[0] == '\\')
        {
            char* endPos = nullptr;
            // 16进制字符 \xA2
            if (tolower(pcontent[1] == 'x'))
            {
                (void)strtol(pcontent + 2, &endPos, 16);
            }
            // 8进制字符 \023
            else if (pcontent[1] >= '0' && pcontent[1] < '8')
            {
                (void)strtol(pcontent + 1, &endPos, 8);
            }


            //进制型转义字符
            if (endPos)
            {
                if (endPos - pcontent > 4)
                {
                    (*pEndCharPos) = pcontent + 4;
                }
                else
                {
                    *pEndCharPos = endPos;
                }
            }
            //一般型转义字符
            else
            {
                *pEndCharPos = pcontent + 2;
            }
        }
        else
        {
            *pEndCharPos = pcontent + 1;
        }
    }

}
