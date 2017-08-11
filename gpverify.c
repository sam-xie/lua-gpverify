/*
* The library offers function to:
*   verify the response data
*   from GOOGLE-PLAY IN-APP BILLING VERSION 3 API purchase request
*   by openssl
*
*/

#include <stddef.h>
#include <string.h>
#include <lua.h>
#include <lauxlib.h>
#include <openssl/bio.h>
#include <openssl/pem.h>
#include <openssl/evp.h>
#include <openssl/err.h>

int openssl_pushresult(lua_State*L, int result) {
	if (result >= 1) {
		lua_pushboolean(L, 1);
		return 1;
	} else if (result == 0) {
		lua_pushboolean(L, 0);
		return 1;
	} else {
		unsigned long val = ERR_get_error();
		lua_pushnil(L);
		if (val) {
			lua_pushstring(L, ERR_reason_error_string(val));
			lua_pushinteger(L, val);
		} else {
			lua_pushstring(L, "UNKNOWN ERROR");
			lua_pushnil(L);
		}
		return 3;
	}
}

BIO* load_bio_object(lua_State* L, int idx) {
	BIO* bio = NULL;
	if (lua_isstring(L, idx))
	{
		size_t sz = 0;
		const char* ctx = lua_tolstring(L, idx, &sz);
		bio = (BIO*)BIO_new_mem_buf((void*)ctx, sz);
	} else {
		luaL_argerror(L, idx, "only support string");
	}
	return bio;
}

/*
 * params:
 *  string      pem content
 *  string      msg
 *  string      sig base64 encoded
 *
 * return:
 *  when successed:
 *      true
 *  when failed(verify failed or incorrect base64 sig):
 *      false
 *  when error:
 *      nil, openssl_err_msg, openssl_err_no
 *
 */
int rsa_verify(lua_State* L) {
	int ret = -1;
	int isPkeyFree = 0;

	// get pem string bio
	BIO* bio = load_bio_object(L, 1);
	if (bio == NULL) {
		goto _err_bio;
	}
	RSA* rsa = PEM_read_bio_RSA_PUBKEY(bio, NULL, 0, NULL);
	if (rsa == NULL) {
		goto _err_rsa;
	}

	// trans bio to pkey
	EVP_PKEY* pkey = EVP_PKEY_new();
	if (pkey == NULL) {
		goto _err_evp;
	}
	if (EVP_PKEY_assign_RSA(pkey, rsa) == 0) {
		goto _err;
	}

	// get message & signature
	size_t msglen;
	const char* msg = luaL_checklstring(L, 2, &msglen);
	size_t sigb64len;
	const char* sigb64 = luaL_checklstring(L, 3, &sigb64len);

	unsigned char* sig = OPENSSL_malloc(sizeof(char) * sigb64len);
	if (sig == NULL) {
		goto _err;
	}
	size_t sigLen = EVP_DecodeBlock(sig, (const unsigned char*)sigb64, sigb64len);
	if (sigLen <= 0) {
		ret = 0;
		goto _err;
	}
	while(sigb64[--sigb64len] == '=') {
		sigLen--;
	}


	// init EVP
	EVP_MD_CTX* md_ctx = EVP_MD_CTX_create();
	if(md_ctx == NULL) {
		goto _err_f;
	}
	if(EVP_VerifyInit(md_ctx, EVP_sha1()) &&
		EVP_VerifyUpdate(md_ctx, msg, msglen)) {
		ret = EVP_VerifyFinal(md_ctx, (unsigned char*)sig, (unsigned int)sigLen, pkey);
	}

	EVP_MD_CTX_destroy(md_ctx);

_err_f:
	OPENSSL_free(sig);
_err:
	EVP_PKEY_free(pkey);
	// incase of double free, EVP_PKEY_free will free rsa
	isPkeyFree = 1;
_err_evp:
	if (!isPkeyFree) {
		RSA_free(rsa);
	}
_err_rsa:
	BIO_free_all(bio);
_err_bio:
	return openssl_pushresult(L, ret);
}


int luaopen_gpverify(lua_State *L) {
	luaL_checkversion(L);

	luaL_Reg reg[] = {
		{"verify", rsa_verify},
		{NULL, NULL}
	};
	luaL_newlib(L, reg);
	return 1;
}
