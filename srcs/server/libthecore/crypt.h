#ifdef __cplusplus
extern "C" {
#endif

/* TEA is a 64-bit symmetric block cipher with a 128-bit key, developed
   by David J. Wheeler and Roger M. Needham, and described in their
   paper at <URL:http://www.cl.cam.ac.uk/ftp/users/djw3/tea.ps>.

   This implementation is based on their code in
   <URL:http://www.cl.cam.ac.uk/ftp/users/djw3/xtea.ps> */

extern int32_t TEA_Encrypt(uint32_t* dest, const uint32_t* src, const uint32_t* key, int32_t size);
extern int32_t TEA_Decrypt(uint32_t* dest, const uint32_t* src, const uint32_t* key, int32_t size);

extern int32_t GOST_Encrypt(uint32_t* DstBuffer, const uint32_t* SrcBuffer, const uint32_t* KeyAddress, uint32_t Length,
                        uint32_t* IVector);
extern int32_t GOST_Decrypt(uint32_t* DstBuffer, const uint32_t* SrcBuffer, const uint32_t* KeyAddress, uint32_t Length,
                        uint32_t* IVector);

extern int32_t DES_Encrypt(uint32_t* DstBuffer, const uint32_t* SrcBuffer, const uint32_t* KeyAddress, uint32_t Length, uint32_t* IVector);
extern int32_t DES_Decrypt(uint32_t* DstBuffer, const uint32_t* SrcBuffer, const uint32_t* KeyAddress, uint32_t Length, uint32_t* IVector);

#ifdef __cplusplus
}
#endif
