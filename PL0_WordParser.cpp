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
            //ע�Ͳ��ֺ���,���򽫴��������ż�
            if (symbol.symbolType != ST_Comment)
            {
                m_Symbols.push_back(symbol);
            }

            //���д��������¼������Ϣ
            if (errorInfo.errorCode != 0)
            {
                m_ErrorInfos.push_back(errorInfo);
            }
        }

        //���޴�����Ϣ����ζ�ŷ����ɹ�
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
        //�Ѿ��������β��
        if (ch == '\0')
        {
            return '\0';
        }
        //�Ѿ��ﵽ��ǰ��ĩ
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

        //ץȡ��һ���ַ�
        char ch;

        //�����з����Ʊ�����ո�
        while (isspace(ch = PeekNextChar()))
        {
            IgnoreNextChar();
        }

        //��¼��ʼ���ݺ�λ��
        const char* startContent = m_pCurrContent;
        symbol.beg = m_CurrCoord;

        ch = GetNextChar();
#pragma region �ؼ��ֻ��ʶ��
        //�������Ϳ����ǹؼ��ֻ��ʶ�� [keyword][indeitfier]
        if (isalpha(ch) || ch == '_')
        {
            ch = PeekNextChar();
            while (isalpha(ch) || isdigit(ch) || ch == '_')
            {
                IgnoreNextChar();
                ch = PeekNextChar();
            }

            //�ж��Ƿ�Ϊ�ؼ���
            std::string word(startContent, m_pCurrContent);
            for (size_t i = 0; i < g_KeywordCount; ++i)
            {
                if (word == g_KeyWords[i])
                {
                    symbol.symbolType = static_cast<SymbolType>(g_KeywordOffset + i);
                    break;
                }
            }

            //���ǹؼ��ֵĻ���Ϊ��ʶ��
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
#pragma region ���ͻ򸡵���
        else if (isdigit(ch))
        {
            //�����ͱ�Ϊ����
            ch = PeekNextChar();
            while (isdigit(ch))
            {
                IgnoreNextChar();
                ch = PeekNextChar();
            }

            symbol.symbolType = ST_Integer;

            //�����ͱ�Ϊ������
            if (ch == '.')
            {
                symbol.symbolType = ST_Float;

                do
                {
                    IgnoreNextChar();
                    ch = PeekNextChar();
                } while (isdigit(ch));

                //��ѧ������
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
                //����ʣ�µ��νӲ���
                do
                {
                    IgnoreNextChar();
                    ch = PeekNextChar();
                } while (isdigit(ch) || isalpha(ch) || ch == '_');

                symbol.symbolType = ST_Null;

            }
        }
#pragma endregion

#pragma region �ַ���
        //�����������ַ��� 'a'
        else if (ch == '\'')
        {
            const char* beginCharStr = m_pCurrContent;
            const char* endCharStr = nullptr;

            //���Է����ַ�
            TryParseChar(beginCharStr, &endCharStr);

            while (endCharStr - m_pCurrContent > 0)
            {
                IgnoreNextChar();
            }
            ch = PeekNextChar();

            if (ch != '\'')
            {
                //ץȡ��һ���ַ�β����β�����
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

            //ɨ����һ�� 'ʱ��Ҫ�ܿ�
            if (ch == '\'')
            {
                //�ַ���û������ ''
                if (m_pCurrContent == beginCharStr)
                {
                    errorInfo.errorCode = 7;            //[Error07] Charcter expected
                }
                //��������ַ�
                else if (m_pCurrContent > endCharStr)
                {
                    errorInfo.errorCode = 11;           //[Error11] Too much Character
                }
                IgnoreNextChar();
            }
        }
#pragma endregion
#pragma region �ַ�����
        //�����ͱ�ʱ�ַ����� "abc"
        else if (ch == '\"')
        {
            const char* beginCharStr = m_pCurrContent;
            const char* endCharStr = beginCharStr + 1;

            //����һֱ�����ַ�
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
#pragma region ��ֵ
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

#pragma region �Ƚ���������߼������
        else if (ch == '=')
        {
            symbol.symbolType = ST_Equal;
        }
        else if (ch == '>')
        {
            ch = PeekNextChar();
            //���������Ǵ��ڵ��� [>=]
            if (ch == '=')
            {
                IgnoreNextChar();
                symbol.symbolType = ST_GreaterEqual;
            }
            //���������Ǵ��ڵ��� [>]
            else
            {
                symbol.symbolType = ST_Greater;
            }
        }
        else if (ch == '<')
        {
            ch = PeekNextChar();
            //���������Ǵ��ڵ��� [<=]
            if (ch == '=')
            {
                IgnoreNextChar();
                symbol.symbolType = ST_LessEqual;
            }
            //���������Ǵ��ڵ��� [<]
            else
            {
                symbol.symbolType = ST_Less;
            }
        }
        else if (ch == '!')
        {
            ch = PeekNextChar();
            //���������Ǵ��ڵ��� [!=]
            if (ch == '=')
            {
                IgnoreNextChar();
                symbol.symbolType = ST_NotEqual;
            }
            //���������Ǵ��ڵ��� [<]
            else
            {
                symbol.symbolType = ST_LogicalNot;
            }
        }
#pragma endregion
#pragma region  �߼������
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
#pragma region һ���������ע��
        else if (ch == '+')
        {
            ch = PeekNextChar();
            //���������ǼӺ�ֵ[+=]
            if (ch == '=')
            {
                IgnoreNextChar();
                symbol.symbolType = ST_PlusAndAssign;
            }
            //��������������++
            else if (ch == '+')
            {
                IgnoreNextChar();
                symbol.symbolType = ST_SelfAdd;
            }
            //���������Ǽӷ�[+]
            else
            {
                symbol.symbolType = ST_Plus;
            }
        }
        else if (ch == '-')
        {
            ch = PeekNextChar();
            //���������Ǽ���ֵ[-=]
            if (ch == '=')
            {
                IgnoreNextChar();
                symbol.symbolType = ST_MinusAndAssign;
            }
            //��������������++
            else if (ch == '-')
            {
                IgnoreNextChar();
                symbol.symbolType = ST_SelfSub;
            }
            //���������Ǽ���[-]
            else
            {
                symbol.symbolType = ST_Minus;
            }
        }
        else if (ch == '*')
        {
            ch = PeekNextChar();
            //���������ǳ˺�ֵ[*=]
            if (ch == '=')
            {
                IgnoreNextChar();
                symbol.symbolType = ST_MultiplyAndAssign;
            }
            //���������ǳ˷�[*]
            else
            {
                symbol.symbolType = ST_Multiply;
            }
        }
        else if (ch == '/')
        {
            ch = PeekNextChar();
            //���������ǳ���ֵ[/=]
            if (ch == '=')
            {
                IgnoreNextChar();
                symbol.symbolType = ST_DivideAndAssign;
            }
            //����������ע��[//]
            else if (ch == '/')
            {
                IgnoreNextChar();
                ch = PeekNextChar();
                while (ch && ch != '\n')
                {
                    // \��ӻ��з�ע����һ��
                    if (ch == '\\')
                    {
                        IgnoreNextChar();
                    }
                    IgnoreNextChar();
                    ch = PeekNextChar();
                }
                symbol.symbolType = ST_Comment;
            }
            //����������ע��[/*.....*/]
            else if (ch == '*')
            {
                IgnoreNextChar();
                ch = PeekNextChar();
                // Ѱ��*/
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
                //���û���ҵ����򱨴�
                if (!ch)
                {
                    errorInfo.errorCode = 4;            //[Error004] Missing*/
                }
                symbol.symbolType = ST_Comment;
            }
            //���������ǳ���[/]
            else
            {
                symbol.symbolType = ST_Divide;
            }
        }
        else if (ch == '%')
        {
            ch = PeekNextChar();
            //����������ȡ���ֵ[%=]
            if (ch == '=')
            {
                IgnoreNextChar();
                symbol.symbolType = ST_ModAndAssign;
            }
            //����������ȡ��[%]
            else
            {
                symbol.symbolType = ST_Mod;
            }
        }
#pragma endregion
#pragma region ����
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
#pragma region �ָ���
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

        //�����з����Ʊ�����ո�
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
        //ת���ַ�
        else if (pcontent[0] == '\\')
        {
            char* endPos = nullptr;
            // 16�����ַ� \xA2
            if (tolower(pcontent[1] == 'x'))
            {
                (void)strtol(pcontent + 2, &endPos, 16);
            }
            // 8�����ַ� \023
            else if (pcontent[1] >= '0' && pcontent[1] < '8')
            {
                (void)strtol(pcontent + 1, &endPos, 8);
            }


            //������ת���ַ�
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
            //һ����ת���ַ�
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
