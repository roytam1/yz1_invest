#ifndef __yz32lib_h__
#define __yz32lib_h__

// time_t
#if !defined(_USE_32BIT_TIME_T)
	#error "Please define _USE_32BIT_TIME_T"
#endif
#include <sys/types.h>
typedef time_t YZtime_t;

// STL
#ifdef WIN32
#pragma warning( disable : 4996 ) // _s
#pragma warning( disable : 4786 )
#pragma warning( disable : 4251 ) // string and vector<string> are not exported !
                                  // so, this static link version must be used
                                  // by the same version of compiler and library
                                  // with the DLL.
#endif // WIN32

#include <vector>
#include <string>
using std::string;
using std::vector;

typedef std::string              FileName ;  // �t�@�C����
typedef std::vector< FileName >  FileTbl  ;  // �t�@�C���ꗗ

#ifndef YZ1DLLEXP
	#ifdef YZ1_STATIC
		#define YZ1DLLEXP
	#elif defined(YZ1_EXPORTS)
		#define YZ1DLLEXP __declspec(dllexport) 
	#else
		#define YZ1DLLEXP __declspec(dllimport) 
	#endif
#endif

//-------------------------------------------------------------------
// yzDecHead �N���X

class yzFileEv;

class YZ1DLLEXP yzDecHead
{
    public: long        m_file_count ;           // �t�@�C����
    public: long *      m_file_size_tbl ;        // �t�@�C���T�C�Y�i�����j
    public: YZtime_t *  m_file_mtime_tbl ;       // �X�V�����i�����j
    public: FileTbl     m_file_name_tbl ;        // �t�@�C�����i�����j
    public: int         m_file_name_max_length ; // �t�@�C�����ő咷
    public: long        m_file_size_max ;        // �t�@�C���T�C�Y�ő�

    private: yzFileEv * yzm ;              // ���b�Z�[�W�o�͐�

    //-----------------------
    //  �R���X�g���N�^
    public:  yzDecHead(
        const unsigned char *  in_area,      // in : �t�@�C���ꗗ�G���A
        const long &           in_file_cnt,  // in : �t�@�C����
        yzFileEv *             yzmev,        // in : ���b�Z�[�W�o�͐�
        const long &           in_size,      // kinaba :: in : in_area�̃T�C�Y(�o�C�g��)
	bool*                  success       // kinaba :: out : �f�R�[�h���������H
    );

    //-----------------------
    //  �f�B���N�g���̓��e�ƃ������̓��e�Ɣ�r
    public: int                             // out: 1:���f, 0:�s��v����(���s), -1:�s��v����(���s)
    MemChk(
        const char *           in_dir_name, // in : �f�B���N�g����
        const int &            in_n,        // in : n �Ԗ�
        const unsigned char *  in_mem       // in : ��r������
    );

    //-----------------------
    //  �f�B���N�g���̓��e��\��
    public: void  Print();

    //-----------------------
    //  ���������e���t�@�C���Ƃ��ďo��
    int                                     // out: 0:OK, !0:���s
    FileWrite(                              // ---:
        const char *           in_dir_name, // in : �J�����g�f�B���N�g��
        const int &            in_n,        // in : �ԍ�
        const unsigned char *  in_data      // in : �o�̓f�[�^�i�W�J�o�f�[�^�j
    );
};


//-------------------------------------------------------------------
// yzFileStat �N���X

class YZ1DLLEXP yzFileStat
{
    public: string  m_file_name ;        // �t�@�C����
    public: long    m_file_size ;        // �t�@�C���T�C�Y
    public: time_t  m_file_mtime ;       // �X�V����

    public: yzFileStat(
        string  in_file_name,        // �t�@�C����
        long    in_file_size,        // �t�@�C���T�C�Y
        time_t  in_file_mtime        // �X�V����
    );
};


//-------------------------------------------------------------------
// yzDecodeHeader �N���X

class YZ1DLLEXP yzDecodeHeader
{
    private : yzDecHead &  m_yz_dec_head ;

    // �R���X�g���N�^
    public : yzDecodeHeader(yzDecHead & in_yz_dec_head );

    public : vector< yzFileStat >  filestat_Get();
};


//-------------------------------------------------------------------
//  yzFileEv �N���X�i yzFileEncode/yzFileDecode ����̃C�x���g�N���X�j

class YZ1DLLEXP yzFileEv
{
    //-----------------------
    //  [!]���b�Z�[�W�o��
    public: virtual void           // out: ����
    MessEX(                        // ---: --------------
        const char *  in_str,      // in : �o�̓��b�Z�[�W
        const int &   in_msg_no    // in : ���b�Z�[�W���ʔԍ�
    ) = 0 ;

    //-----------------------
    //  [i]���b�Z�[�W�o��
    public: virtual void           // out: ����
    MessINF(                       // ---: --------------
        const char *  in_str,      // in : �o�̓��b�Z�[�W
        const int &   in_msg_no    // in : ���b�Z�[�W���ʔԍ�
    ) = 0 ;

    //-----------------------
    //  [?]���b�Z�[�W�o��
    public: virtual int            // out: 1:no, 0:yes
    MessYN(                        // ---: --------------
        const char *  in_str,      // in : �o�̓��b�Z�[�W
        const int &   in_msg_no    // in : ���b�Z�[�W���ʔԍ�
    ) = 0 ;

    //-----------------------
    //  [INPUT]���b�Z�[�W�o��
    public: virtual char *         // out: ���͕�����
    MessINPUT(                     // ---: --------------
        const char *  in_str,      // in : �o�̓��b�Z�[�W
        const int &   in_msg_no    // in : ���b�Z�[�W���ʔԍ�
     ) = 0 ;

    //-----------------------
    //  Gauge ������
    public: virtual int                     // out: 0:���s, !0:���f
    gauge_EncodeInit(                       // ---: --------------
        const char *   in_input_file_name,  // in : ���̓t�@�C���i�f�B���N�g���j���i�t���p�X�j
        FileName &     output_name,         // in/out: �o�̓t�@�C�����i�u�������\�j
        FileTbl &      fullpath_list,       // in/out: �t���p�X�t�@�C�����ꗗ�i�u�������\�j
        FileTbl &      file_list            // in/out: ���΃t�@�C�����ꗗ�i�u�������\�j
    ) = 0 ;

    //-----------------------
    //  Gauge ������
    public: virtual int                        // out: 0:���s, !0:���f
    gauge_DecodeInit(                          // ---: --------------
        const char *            in_file_name,  // in : �𓀂���t�@�C����
        const yzDecodeHeader &  in_yz_dec_head // in : �W�J���
    ) = 0 ;

    //-----------------------
    //  Gauge �Q�[�W�̕�����\��
    public: virtual int            // out: 0:���s, !0:���f
    gauge_String(                  // ---: --------------
        const char *  in_str       // in : ���b�Z�[�W
    ) = 0 ;

    //-----------------------
    //  Gauge �Q�[�W�̒l�\��
    public: virtual int            // out: 0:���s, !0:���f
    gauge_Value(                   // ---: --------------
        const int &  in_per,       // in : �o�ߊ��� �p�[�Z���g 0-1000
        const int &  in_end_time   // in : �I���\�z���� 1/100 �b��
    ) = 0 ;

    //-----------------------
    //  Gauge �t�@�C���o�͒��O���
    public: virtual int                 // out: 0:�o�͑��s, 1:skip(�o�͂������s), 2:���f
    gauge_WriteInfo(                    // ---: --------------
        const yzFileStat &  in_yzfstat, // in : �t�@�C���ڍ׏��
        const int &         in_no,      // in : �������̃t�@�C���ԍ��i0�`�j
        const int &         in_max      // in : �S�t�@�C����
    ) = 0 ;
};


//-------------------------------------------------------------------

class YZ1DLLEXP YzMessage
{
    //-----------------------
    // ���b�Z�[�W����i���{��/�p��j
    public: enum language_type
    {
        language_japanese = 1, // ���{��
        language_english  = 2, // �p��
    } ;

    public: static language_type  language ;
};

#include "YZ_MsgTbl.h"


//-------------------------------------------------------------------

enum YZ_MAIN_ENCODE_PASSWD_MODE    // �p�X���[�h�̎g�p
{
    YZ_MAIN_ENCODE_PASSWD_OFF = 0, // �p�X���[�h�n�e�e
    YZ_MAIN_ENCODE_PASSWD_ON  = 1, // �p�X���[�h�n�m
    YZ_MAIN_ENCODE_PASSWD_PUB = 2  // ���J�`���p�X���[�h�n�m
} ;


//-------------------------------------------------------------------
//  yzFileEncode �N���X

class YZ1DLLEXP yzFileEncode
{
    public:  FileName          m_output_name ;  // ���k��̏o�̓t�@�C����

    private: yzFileEv *        yzm ;            // ���b�Z�[�W�o�͐�
    private: const char *      dirName ;        // ���k�t�@�C���o�̓f�B���N�g��
    private: const char *      SelfEncodeFile ; // ���ȉ𓀃t�@�C���쐬
    private: const enum YZ_MAIN_ENCODE_PASSWD_MODE  passwd_mode ; // �p�X���[�h�̎g�p

    //-----------------------
    //  �R���X�g���N�^
    public: yzFileEncode(
        yzFileEv *    in_yzm,                     // in : ���b�Z�[�W�o�͐�
        const char *  in_dir_name         = NULL, // in : ���k�t�@�C���o�̓f�B���N�g��
        const char *  in_self_encode_file = NULL, // in : ���ȉ𓀃t�@�C���쐬
        const enum YZ_MAIN_ENCODE_PASSWD_MODE  in_passwd_mode = // in : �p�X���[�h�̎g�p
             YZ_MAIN_ENCODE_PASSWD_OFF                          //      �p�X���[�h�n�e�e
    );

    //-------------------------------------------------------------------
    //  �u���k�v  -1:�L�����Z��, 0�ȏ�:����I��
    public:    int                   // out: -1:�L�����Z��, 0�ȏ�:����I��
    EncodeFile(
        const char *  in_file_name   // in : ���k����t�@�C����
    );
};


//-------------------------------------------------------------------
//  yzFileDecode �N���X

class MyBitGetEv ;  // ���k�f�[�^�ǂݍ���
class MemAlloc ;    // ���k�f�[�^�̃w�b�_�ǂݍ��݃G���A
class yzDecode ;    // ���k�f�[�^�̉�

class YZ1DLLEXP yzFileDecode
{
friend class yzDLLArcHandler;		// ����ɉ��� by K.INABA
friend class yzDLLArcHeaderHandler;	// ����ɉ��� by K.INABA

    public:  char *       yzfiledecode_output_dir_name ; // �L����A�f�B���N�g����
    public:  yzDecHead *  yzfiledecode_p_yz_dec_head ;   // ���k�f�[�^�̃w�b�_���

    private: yzFileEv *   yzm ;        // ���b�Z�[�W�o�͐�

    private: MyBitGetEv *  p_bit_get ;  // ���k�f�[�^�ǂݍ���
    private: MemAlloc *    p_head ;     // ���k�f�[�^�̃w�b�_�ǂݍ��݃G���A
    private: yzDecode *    p_yz_decode ;// ���k�f�[�^�̉�
    private: long  dec_all_size   ; // 2: AllSize(4)
    private: long  dec_head_size  ; // 3: HeaderSize(4)
    private: long  dec_file_count ; // 4: FileCount(4)
    private: long  dec_data_size  ;


	//-------------------------------------------------------------------
	//  �R���X�g���N�^
	public: yzFileDecode(
	    yzFileEv *    in_yzm,        // in : ���b�Z�[�W�o�͐�
	    const char *  in_dir_name    // in : �L����A�f�B���N�g����
	);

	//-------------------------------------------------------------------
	//  �f�X�g���N�^
	// �f�X�g���N�^���K�v�� class �̓R�s�[���֎~����B
	private: yzFileDecode            (const yzFileDecode&) ; // �I�u�W�F�N�g�R�s�[�֎~
	private: yzFileDecode & operator=(const yzFileDecode&) ; // �I�u�W�F�N�g�R�s�[�֎~
	public: ~yzFileDecode();

	//-------------------------------------------------------------------
	//  YZ�t�@�C���̃w�b�_���̂݉�
	public: int                              // out:-1:�L�����Z��, 0�ȏ�:����I��
	YzFile_DecodeHeader(
	    const char *  in_file_name   // in : �L������t�@�C����
	);

	//-------------------------------------------------------------------
	//  YZ�t�@�C���̓��e�\��
	public: int                      // out: -1:�L�����Z��, 0�ȏ�:����I��
	Print(                           // ---: ----------------------------
	    const char *  fileName       // in : �L������t�@�C����
	);

	//-------------------------------------------------------------------
	//  YZ�t�@�C���̉�
	public: int                      // out: -1:�L�����Z��, 0�ȏ�:����I��
	DecodeFile(                      // ---: ----------------------------
	    const char *  fileName,      // in : �L������t�@�C����
	    const int &   in_decode_mode // in : �L�����[�h 0:�������ށA1:�������܂Ȃ��i�𓀃e�X�g�j
	);
};

#ifdef WIN32
#pragma warning( default : 4786 )
#pragma warning( default : 4251 )
#endif // WIN32
#endif // __yz32lib_h__
