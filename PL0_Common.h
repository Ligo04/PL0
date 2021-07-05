#pragma once
#include<array>
#include<vector>
#include<set>
#include<forward_list>
#include<stack>
#include<string>


#pragma warning(disable:26812)
#pragma warning(disable:26495)


namespace PL0
{
	//����
	constexpr size_t g_KeywordOffset = 100;			//�ؼ�������ڷ�������ö��ֵ��ƫ����
	constexpr size_t g_KeywordCount = 20;			//�ؼ�����Ŀ
	constexpr size_t g_MaxIdentifierLength = 47;	//��ʶ����󳤶�
	constexpr size_t g_MaxIntegerLength = 10;		//������󳤶�


	//��������
	enum SymbolType
	{
		ST_Null = 0,			//��
		ST_Error,				//��������
		//ֵ����
		ST_Identifier=10,		//��ʶ��
		ST_Integer,				//����
		ST_Real,				//ʵ��
		ST_Character,			//�ַ���
		ST_StringConst,			//�ַ�������

		//���������
		ST_Plus=20,				//��+
		ST_Minus,				//��-
		ST_Multiply,			//��*
		ST_Divide,				//��/
		ST_Mod,					//ȡ��%

		//��ֵ
		ST_Assign=30,			//��ֵ��� :=
		ST_PlusAndAssign,		//�Ӻ�ֵ +=
		ST_MinusAndAssign,		//����ֵ -=
		ST_MultiplyAndAssign,	//�˺�ֵ *=
		ST_DivideAndAssign,		//����ֵ /=
		ST_ModAndAssign,		//ȡ���ֵ %=

		ST_SelfAdd,				//����++
		ST_SelfSub,				//�Լ�--

		//�Ƚ������
		ST_Equal=40,			//�ж� lhs == rhs
		ST_NotEqual,			//�ж� lhs != rhs
		ST_Less,				//�ж� lhs < rhs
		ST_LessEqual,			//�ж� lhs <= rhs
		ST_Greater,				//�ж� lhs > rhs
		ST_GreaterEqual,	    //�ж� lhs >= rhs

		//�߼������
		ST_LogicalAnd=50,		// �߼��� &&
		ST_LogicalOr,			// �߼��� ||
		ST_LogicalNot,			// �߼��� !

		//����
		ST_LeftParen=60,		// С���� ��
		ST_RightParen,			// С���� �� ��
		ST_LeftBracket,			// ������ �� [
		ST_RightBracket,		// ������ �� ]
		ST_LeftBrace,			// ������ �� {
		ST_RightBrace,			// ������ �� }

		//�ָ��
		ST_Comma=70,			// ���� ��
		ST_SemiColon,			// �ֺ� ��
		ST_Period,				// ��� .

		//ע��
		ST_Comment=80,			// ע��

		//�ؼ���
		ST_Begin=100,			//����Ƭ�ο�ʼ begin
		ST_Call,				//��������
		ST_Char,				//char�������
		ST_Const,				//�����������η�
		ST_Do,					//do���
		ST_Else,				//else���
		ST_End,					//����Ƭ�ν��� end
		ST_Float,				//float�������
		ST_For,					//for���
		ST_If,					//if���
		ST_Int,					//int�������
		ST_Procedure,			//�����������η�
		ST_Read,				//read���
		ST_Return,				//return���
		ST_Step,				//step���
		ST_String,				//string���
		ST_Then,				//then���
		ST_Until,				//until���
		ST_While,				//while���
		ST_Write,				//write���
	};

	//��������
	enum FuncType
	{
		Func_LIT,				//ȡ������
		Func_OPR,				//����
		Func_LOD,			    //��ȡ
		Func_STO,				//����
		Func_CAL,				//����
		Func_INT,				//��ʼ���ռ�
		Func_POP,				//��ջ
		Func_JMP,				//��������ת
		Func_JPC				//��������ת
	};

	// ��������
	enum OprType
	{
		Opr_RET,				// ���̷���
		Opr_NEG,				// ջ��:= -ջ��
		Opr_NOT,				// ջ��:= !ջ��
		Opr_SAD,				// ջ��:= ջ�� + 1
		Opr_SSU,				// ջ��:= ջ�� - 1
		Opr_ADD,				// ջ��:= ��ջ�� + ջ�� t��1
		Opr_SUB,				// ջ��:= ��ջ�� - ջ�� t��1
		Opr_MUL,				// ջ��:= ��ջ�� * ջ�� t��1
		Opr_DIV,				// ջ��:= ��ջ�� / ջ�� t��1
		Opr_MOD,				// ջ��:= ��ջ�� % ջ�� t��1
		Opr_EQU,				// ջ��:= (��ջ�� = ջ��) t��1
		Opr_NEQ,				// ջ��:= (��ջ�� != ջ��) t��1
		Opr_LES,				// ջ��:= (��ջ�� < ջ��) t��1
		Opr_LEQ,				// ջ��:= (��ջ�� <= ջ��) t��1
		Opr_GTR,				// ջ��:= (��ջ�� > ջ��) t��1
		Opr_GEQ,				// ջ��:= (��ջ�� >= ջ��) t��1
		Opr_PRT,				// ջ����ֵ���������̨��Ļ t��1
		Opr_PNL,				// ����̨��Ļ���һ������
		Opr_SCN					// �ӿ���̨��ȡ���룬ֲ��ջ�� t��1
	};


	//��ʶ������
	enum IDType
	{
		ID_INT = 0x1,
		ID_FLOAT = 0x2,
		ID_PROCEDURE = 0x4,

		ID_CONST = 0x10,
		ID_VAR = 0X20,
		ID_PARAMETER = 0x40
	};

	//�ؼ����ַ���
	const char* const g_KeyWords[g_KeywordCount] =
	{
		"begin",
		"call",
		"char",
		"const",
		"do",
		"else",
		"end",
		"float",
		"for",
		"if",
		"int",
		"procedure",
		"read",
		"return",
		"step",
		"string",
		"then",
		"until",
		"while",
		"write"
	};


	//������
	using ErrorCode = uint32_t;

	//�������꣨��1��ʼ��
	struct CodeCoord
	{
		size_t row;
		size_t col;
	};

	//���ţ���ɴʷ�������
	struct Symbol
	{
		std::string word;
		CodeCoord beg, end;
		SymbolType symbolType;
	};


	struct ErrorInfo
	{
		CodeCoord beg, end;
		ErrorCode errorCode;
	};

	// ��ʶ��(����﷨����)
	struct Identifier
	{
		char name[g_MaxIdentifierLength + 1];	// ����
		uint32_t kind;							// ��������
		int value;								// ֵ
		int level;								// �㼶
		int offset;								// ��ַƫ��
	};

	// ָ��
	struct Instruction
	{
		uint32_t func;		// ����
		int level;			// �㼶��
		int mix;			// ��ַƫ��/������/ָ��
		int reserved;		// �����֣���ʱ���ã�
	};

	// ������Ϣ
	struct ProgramInfo
	{
		// [4�ֽ�]��ʶ����ʼλ�� [4�ֽ�]��ʶ����Ŀ [4�ֽ�]ָ����ʼλ�� [4�ֽ�]ָ����Ŀ
		// {
		//   [64�ֽ�] ���̱�ʶ��
		//   { [64�ֽ�] �ù��̵��β�}
		//   { [64�ֽ�] �ù��̵ı���}
		// }
		//
		// {[16�ֽ�] ָ��}

		std::vector<Identifier> identifiers;	// ��ʶ��
		std::vector<Instruction> instructions;	// ָ��
	};

	// ������Ϣ
	struct EnvironmentInfo
	{
		int pr;								// ָ���ַ�Ĵ���
		int tr;								// ջ���Ĵ���
		int br;								// ��ַ�Ĵ���
		Instruction ir;						// ָ��Ĵ���
		std::vector<int> dataStack;			// ����ջ��
		std::stack<int> prStack;			// ָ���ַ�Ĵ�ջ
		std::stack<int> trStack;			// ջ���Ĵ�ջ
		std::stack<int> brStack;			// ��ַ�Ĵ�ջ
		std::vector<std::string> funcStack;	// ��������ջ
	};
}