/* BEGIN: composite_pmeth.c */

// Temporary Measure until the functions are all used
#pragma GCC diagnostic ignored "-Wunused-function"

// Composite Crypto authentication methods.
// (c) 2021 by Massimiliano Pala

#ifndef _LIBPKI_COMPOSITE_PKEY_METH_H
#include <libpki/openssl/composite/composite_pmeth.h>
#endif

#ifndef _LIBPKI_COMPOSITE_UTILS_H
#include <libpki/openssl/composite/composite_utils.h>
#endif

// ===============
// Data Structures
// ===============

#ifndef _LIBPKI_COMPOSITE_OPENSSL_LOCAL_H
#include "composite_ossl_internals.h"
#endif

// ======================
// MACRO & Other Oddities
// ======================

#define DEBUG(args...) \
  { fprintf(stderr, "[%s:%d] %s() - ", __FILE__, __LINE__, __func__); \
  fprintf(stderr, ## args) ; fprintf(stderr,"\n"); fflush(stderr) ; }

// ========================
// Exported Global Variable
// ========================

// Temporary Measure until the functions are all used
#pragma GCC diagnostic ignored "-Wunused-function"

#ifdef ENABLE_COMPOSITE

// =======================
// COMPOSITE_KEY Functions
// =======================

int COMPOSITE_KEY_size(COMPOSITE_KEY * key) {

  int i = 0;
  int key_num = 0;  
  int total_size = 0;

  // Input Checks
  if (!key) return -1;

  // Retrieves the number of components
  if ((key_num = COMPOSITE_KEY_num(key)) <= 0) return PKI_ERR;

  // Process the individual keys
  for (i = 0; i < key_num; i++) {

    const EVP_PKEY * single_key = NULL;
      // Pointer to the component

    // Retrieves the component
    if ((single_key = COMPOSITE_KEY_get0(key, i)) == NULL) {
      PKI_DEBUG("ERROR: Cannot get key %d", i);
      return 0;
    }

    // Updates the total size
    total_size += EVP_PKEY_size(single_key);
  }

  // All Done
  return total_size;
}

int COMPOSITE_KEY_bits(COMPOSITE_KEY * key) {

  int i = 0;
  int key_num = 0;  
  int total_bits = 0;

  // Input Checks
  if (!key) return -1;

  // Returns '0' if no components were found
  if ((key_num = COMPOSITE_KEY_num(key)) <= 0) return 0;

  // Process the individual components
  for (i = 0; i < key_num; i++) {

    const EVP_PKEY * single_key = NULL;
      // Pointer for the component

    if ((single_key = COMPOSITE_KEY_get0(key, i)) == NULL) {
      PKI_DEBUG("ERROR: Cannot get key %d", i);
      return -1;
    }

    // Updates the total size
    total_bits += EVP_PKEY_bits(single_key);
  }

  // Total bits
  return total_bits;
}

int COMPOSITE_KEY_security_bits(COMPOSITE_KEY * key) {

  int i = 0;
  int key_num = 0;  
  int sec_bits = INT_MAX;
  int component_sec_bits = INT_MAX;

  // Input checks
  if (!key) return -1;

  // Checks we have at least one component
  if ((key_num = COMPOSITE_KEY_num(key)) <= 0) return -1;

  // Cycles through all the components
  for (i = 0; i < key_num; i++) {

    const EVP_PKEY * single_key;
      // Pouinter to the individual component

    // Retrieves the component key
    if ((single_key = COMPOSITE_KEY_get0(key, i)) == NULL) {
      PKI_DEBUG("ERROR: Cannot get key %d", i);
      return -1;
    }

    // Retrieves the security bits for the component
    component_sec_bits = EVP_PKEY_security_bits(single_key);

    // Updates the composite security bits if the component's
    // strength is higher than the previous components
    if (sec_bits < component_sec_bits) sec_bits = component_sec_bits;
  }

  // All Done
  return sec_bits;
}

// =======================
// COMPOSITE_CTX Functions
// =======================

COMPOSITE_CTX * COMPOSITE_CTX_new_null() {

  COMPOSITE_CTX * ret = NULL;
    // Return pointer

  // Allocates the needed memory
  ret = PKI_Malloc(sizeof(COMPOSITE_CTX));
  if (!ret) return NULL;

  // Initializes the data structure
  ret->md = NULL;

  // Initializes the stack of components
  ret->components = COMPOSITE_KEY_new_null();
  
  // All Done
  return ret;
}

void COMPOSITE_CTX_free(COMPOSITE_CTX * comp_ctx) {

  // Input checks
  if (!comp_ctx) return;

  // Free the memory associated with the
  // component's contexts
  if (comp_ctx->components) {

    PKI_X509_KEYPAIR_VALUE * evp_pkey = NULL;
      // Pointer to the individual context

    // Pop each CTX of the components
    while ((evp_pkey = COMPOSITE_KEY_pop(comp_ctx->components))) {
      // Free the associated memory
      EVP_PKEY_free(evp_pkey);
    }

    // Free the memory of the stack
    COMPOSITE_KEY_free(comp_ctx->components);
  }

  // Free the memory
  PKI_ZFree(comp_ctx, sizeof(COMPOSITE_CTX));
}

COMPOSITE_CTX * COMPOSITE_CTX_new(const EVP_MD * md) {

  COMPOSITE_CTX * ret = NULL;
    // Return Pointer

  // Allocates and Initializes the CTX
  if ((ret = COMPOSITE_CTX_new_null()) != NULL) {
    ret->md = md;
  }
  
  // All Done
  return ret;
}

int COMPOSITE_CTX_set_md(COMPOSITE_CTX * ctx, const EVP_MD * md) {

  // Input Checks
  if (!ctx || !md) return PKI_ERR;

  // Sets the MD
  ctx->md = md;

  // All Done
  return PKI_OK;
}

const EVP_MD * COMPOSITE_CTX_get_md(COMPOSITE_CTX * ctx) {

  // Input checks
  if (!ctx) return NULL;

  // Returns the internal pointer
  return ctx->md;
}

int COMPOSITE_CTX_pkey_push(COMPOSITE_CTX * comp_ctx, PKI_X509_KEYPAIR_VALUE * pkey) {

  // Input Checks
  if (!comp_ctx || !pkey) 
    return PKI_ERROR(PKI_ERR_PARAM_NULL, NULL);

  // Gets the reference to the stack
  if (!comp_ctx->components) {
    return PKI_ERROR(PKI_ERR_MEMORY_ALLOC, "Missing internal stack of keys in CTX");
  }

  // Pushes the new context
  COMPOSITE_KEY_push(comp_ctx->components, pkey);

  // All Done
  return PKI_OK;
}

PKI_X509_KEYPAIR_VALUE * COMPOSITE_CTX_pkey_pop(COMPOSITE_CTX * comp_ctx) {

  // Input Checks
  if (!comp_ctx || !comp_ctx->components)
    return NULL;

  // Pops and returns the last component
  return COMPOSITE_KEY_pop(comp_ctx->components);
}

int COMPOSITE_CTX_pkey_clear(COMPOSITE_CTX * comp_ctx) {

  // Input Checks
  if (!comp_ctx) return PKI_ERR;

  // Clears the components
  if (comp_ctx->components) COMPOSITE_KEY_clear(comp_ctx->components);
  
  // All Done
  return PKI_OK;
}

STACK_OF(EVP_PKEY) * COMPOSITE_CTX_pkey_stack0(COMPOSITE_CTX * ctx) {

  // Input Checks
  if (!ctx) return NULL;

  // Returns the reference to the stack
  return ctx->components;
}

// =========================
// EVP_PKEY_METHOD Functions
// =========================

// Implemented
static int init(EVP_PKEY_CTX *ctx) {
  
  COMPOSITE_CTX *comp_ctx = NULL;

  // Allocate Memory
  if ((comp_ctx = COMPOSITE_CTX_new_null()) == NULL) return 0;

  // Assigns the algorithm-specific data
  // to the data field
  ctx->data = comp_ctx;

  // These are used during Key Gen to display
  // '.', '+', '*', '\n' during key gen
  ctx->keygen_info = NULL;
  ctx->keygen_info_count = 0;

  // All Done
  return 1;
}

// Implemented
static void cleanup(EVP_PKEY_CTX * ctx) {

  // Input Check
  if (!ctx) return;

  // Retrieves the internal context
  if (ctx->data) COMPOSITE_CTX_free((COMPOSITE_CTX *)ctx->data);
  ctx->data = NULL;

  // All Done
  return;
}

// // Not Implemented
// static int paramgen_init(EVP_PKEY_CTX * ctx) {
//   PKI_DEBUG("Not implemented, yet.");
//   return 0;
// }

// // Not Implemented
// static int paramgen(EVP_PKEY_CTX * ctx,
//                     EVP_PKEY     * pkey) {
//   PKI_DEBUG("Not implemented, yet.");
//   return 0;
// }

// Implemented
static int keygen(EVP_PKEY_CTX *ctx, EVP_PKEY *pkey) {

  COMPOSITE_CTX * comp_ctx = NULL;
  COMPOSITE_KEY * key = NULL;

  // Input Validation
  if (!ctx || !ctx->data || !pkey) return 0;

  // // Some extra checking for correctness
  // if (ctx->pmeth->pkey_id != OBJ_txt2nid(OPENCA_ALG_PKEY_EXP_COMP_OID)) {
  //   PKI_DEBUG("NID is not NID_composite (%d vs. %d)", 
  //     ctx->pmeth->pkey_id, OBJ_txt2nid(OPENCA_ALG_PKEY_EXP_COMP_OID));
  //   return 0;
  // }

  // Checks we have the right data and items
  comp_ctx = ctx->data;
  if (!comp_ctx) {
    // No components present in the key
    PKI_ERROR(PKI_ERR_ALGOR_SET, "Empty Stack of Keys when generating a composed key");
    return 0;
  }

  // // Allocates the Composite Key
  // if ((key = COMPOSITE_KEY_new_null()) == NULL) {
  //   PKI_ERROR(PKI_ERR_MEMORY_ALLOC, NULL);
  //   return 0;
  // }

  // // Processes
  // for (int i = 0; i < COMPOSITE_CTX_num(comp_ctx); i++ ) {

  //   EVP_PKEY * tmp_pkey = NULL;
  //     // Pointer to the single component's key

  //   // Retrieves the i-th component
  //   if (!COMPOSITE_CTX_pkey_get0(comp_ctx, &tmp_pkey, i) || (tmp_pkey == NULL)) {
  //     PKI_ERROR(PKI_ERR_MEMORY_ALLOC, "Cannot add PKEY to Composite Key component #%d", i);
  //     COMPOSITE_KEY_free(key);
  //     return 0;
  //   }

  //   // Adds the key in the key stack
  //   COMPOSITE_KEY_push(key, tmp_pkey);
  // }

  // Transfer the components from the CTX
  key = comp_ctx->components;

  // Resets the list of components on the CTX
  comp_ctx->components = COMPOSITE_KEY_new_null();

  // NOTE: To Get the Structure, use EVP_PKEY_get0(EVP_PKEY *k)
  // NOTE: To Add the Key Structure, use EVP_PKEY_assign()
  EVP_PKEY_assign_COMPOSITE(pkey, key);

  // All Done.
  return 1;
}

// Implemented
static int sign(EVP_PKEY_CTX        * ctx, 
                unsigned char       * sig,
                size_t              * siglen,
                const unsigned char * tbs,
                size_t                tbslen) {

  // NOTE: The passed CTX (ctx->data) is not the same as when the key
  // was created or loaded. This means that the comp_ctx that is
  // available here is actually empty. We need to reconstruct the
  // different EVP_PKEY_CTX here.

  COMPOSITE_KEY * comp_key = EVP_PKEY_get0(ctx && ctx->pkey ? ctx->pkey : NULL);
    // Pointer to inner key structure

  EVP_PKEY_CTX * pkey_ctx = NULL;
  EVP_PKEY * evp_pkey = NULL;
    // The keypair and context references

  EVP_MD_CTX * md_ctx = NULL;
    // Digest Context

  const int signature_size = EVP_PKEY_size(ctx->pkey); /* WARN: This does not account for extra space for parameters */
    // The total signature size

  STACK_OF(ASN1_TYPE) *sk = NULL;
    // Stack of ASN1_OCTET_STRINGs

  ASN1_BIT_STRING * bit_string = NULL;
    // Output Signature to be added
    // to the stack of signatures

  ASN1_TYPE * aType = NULL;
    // ASN1 generic wrapper

  int comp_key_num = 0;
    // Number of components

  unsigned char * buff = NULL;
  unsigned char * pnt  = NULL;
  int buff_len =  0;
    // Temp Pointers

  int ret_code = 0;
    // Return Code for external calls

  int total_size = 0;
    // Total Signature Size

  // Input Checks
  comp_key_num = COMPOSITE_KEY_num(comp_key);
  if (comp_key_num <= 0) {
    PKI_ERROR(PKI_ERR_MEMORY_ALLOC, "Cannot get the Composite key inner structure");
    return 0;
  }

  // Input checks -> Destination Buffer Pointer
  if (sig == NULL) {
    *siglen = (size_t)signature_size;
    return 1;
  }

  // Allocates the Stack for the signatures
  if ((sk = sk_ASN1_TYPE_new_null()) == NULL) {
    PKI_ERROR(PKI_ERR_MEMORY_ALLOC, "Cannot allocate the stack of signature");
    goto err;
  }

  // Generates Each Signature Independently
  for (int idx = 0; idx < comp_key_num; idx++) {

    // Retrieves the i-th component
    if ((evp_pkey = COMPOSITE_KEY_get0(comp_key, idx)) == NULL) {
      PKI_ERROR(PKI_ERR_MEMORY_ALLOC, "Cannot get %d-th component from Key", idx);
      goto err;
    }

    // Let's build a PKEY CTX and assign it to the MD CTX
    pkey_ctx = EVP_PKEY_CTX_new(evp_pkey, NULL);
    if (!pkey_ctx) {
      PKI_ERROR(PKI_ERR_MEMORY_ALLOC, "Cannot allocate the %d PKEY CTX component", idx);
      goto err;
    }

    // Gets the Signature's Max Size
    buff_len = EVP_PKEY_size(evp_pkey);

    // Allocate the buffer for the single signature
    if ((pnt = buff = OPENSSL_malloc((size_t)buff_len)) == NULL) {
      PKI_ERROR(PKI_ERR_MEMORY_ALLOC, "Cannot allocate the %d-th component signature's buffer");
      goto err;
    }

    // Initializes the Signing process
    ret_code = EVP_PKEY_sign_init(pkey_ctx);
    if (ret_code != 1) {
      PKI_ERROR(PKI_ERR_SIGNATURE_CREATE, 
        "Cannot initialize %d component signature (EVP_PKEY_sign_init code %d)", 
        idx, ret_code);
      goto err;
    }

    // Signature's generation
    ret_code = EVP_PKEY_sign(pkey_ctx, pnt, (size_t *)&buff_len, tbs, tbslen);
    if (ret_code != 1) {
      PKI_ERROR(PKI_ERR_SIGNATURE_CREATE, 
        "Cannot initialize signature for %d component (EVP_PKEY_sign code is %d)", 
        idx, ret_code);
      goto err;
    }

    // Removes the reference to the key. This is
    // needed because we otherwise will have memory
    // issue when calling EVP_PKEY_CTX_free()
    pkey_ctx->pkey = NULL;

    // Free the PKEY context
    if (pkey_ctx) EVP_PKEY_CTX_free(pkey_ctx);
    pkey_ctx = NULL; // Safety

    // Updates the overall real size
    total_size += buff_len;

    PKI_DEBUG("Generated Signature for Component #%d Successfully (size: %d)", idx, buff_len);
    PKI_DEBUG("Signature Total Size [So Far] ... %d", total_size);

    if ((bit_string = ASN1_BIT_STRING_new()) == NULL) {
      PKI_ERROR(PKI_ERR_MEMORY_ALLOC, 
                "Cannot allocate the wrapping OCTET STRING for signature's %d component",
                idx);
      goto err;
    }

    // This sets the internal pointers
    ASN1_STRING_set0(bit_string, pnt, buff_len);
    pnt = NULL; buff_len = 0;

    // Resets the pointer and length after ownership transfer
    buff = NULL; buff_len = 0;

    // Let's now generate the ASN1_TYPE and add it to the stack
    if ((aType = ASN1_TYPE_new()) == NULL) {
      PKI_ERROR(PKI_ERR_MEMORY_ALLOC, "Cannot Allocate a new ASN1 Type for signature wrapping");
      goto err;
    }

    // Transfer Ownership to the aType structure
    ASN1_TYPE_set(aType, V_ASN1_BIT_STRING, bit_string);
    bit_string = NULL;

    // Adds the component to the stack
    if (!sk_ASN1_TYPE_push(sk, aType)) {
      PKI_ERROR(PKI_ERR_SIGNATURE_CREATE, "Cannot push the signature's %d component", idx);
      goto err;
    }

    // Transfers ownership
    aType = NULL;
  }

  if ((*siglen = (size_t) i2d_ASN1_SEQUENCE_ANY(sk, &sig)) <= 0) {
    PKI_ERROR(PKI_ERR_DATA_ASN1_ENCODING, "Cannot generate DER representation of the sequence of signatures");
    goto err;
  }

  // Reporting the total size
  PKI_DEBUG("Total Signature Size: %d (estimated: %d)", *siglen, signature_size);

  // Free the stack's memory
  while ((aType = sk_ASN1_TYPE_pop(sk)) == NULL) {
    ASN1_TYPE_free(aType);
  } sk_ASN1_TYPE_free(sk);
  sk = NULL;

  // Success
  return 1;

err:
  // Debugging
  PKI_ERROR(PKI_ERR_SIGNATURE_CREATE, NULL);

  // Free allocated memory
  if (md_ctx) EVP_MD_CTX_free(md_ctx);
  if (bit_string) ASN1_OCTET_STRING_free(bit_string);
  if (buff && buff_len) PKI_ZFree(buff, (size_t) buff_len);
  if (pkey_ctx) {
    pkey_ctx->pkey = NULL;
    EVP_PKEY_CTX_free(pkey_ctx);
  }
  // if (evp_pkey) EVP_PKEY_free(evp_pkey);

  // Handles the stack of signatures
  if (sk) {
    while(sk_ASN1_TYPE_num(sk) > 0) { 
      sk_ASN1_TYPE_pop_free(sk, ASN1_TYPE_free);
    }
    if (sk) sk_ASN1_TYPE_free(sk);
  }

  // Error
  return 0;
}

// // Not Implemented
// static int verify_init(EVP_PKEY_CTX *ctx) {
//   PKI_DEBUG("Not implemented, yet.");
//   return 0;
// }

// Implemented
static int verify(EVP_PKEY_CTX        * ctx,
                  const unsigned char * sig,
                  size_t                siglen,
                  const unsigned char * tbs,
                  size_t                tbslen) {

  COMPOSITE_KEY * comp_key = EVP_PKEY_get0(ctx && ctx->pkey ? ctx->pkey : NULL);
    // Pointer to inner key structure

  STACK_OF(ASN1_TYPE) *sk = NULL;
    // Stack of ASN1_OCTET_STRINGs

  ASN1_TYPE * aType = NULL;
    // ASN1 generic wrapper

  int ret_code = 0;
    // OSSL return code

  int comp_key_num = 0;
    // Number of components

  ASN1_BIT_STRING aBitStr;
    // Temp Bit String

  EVP_PKEY_CTX * pkey_ctx = NULL;
  EVP_PKEY * evp_pkey = NULL;
    // The keypair and context references

  // Input Checks
  comp_key_num = COMPOSITE_KEY_num(comp_key);
  if (comp_key_num <= 0) {
    PKI_ERROR(PKI_ERR_MEMORY_ALLOC, "Cannot get the Composite key inner structure");
    return 0;
  }

  // Let's use the aOctetStr to avoid the internal
  // p8 pointers to be modified
  aBitStr.data = (unsigned char *)sig;
  aBitStr.length = (int) siglen;

  // Gets the Sequence from the data itself, error if
  // it is not a sequence of ASN1_OCTET_STRING
  if ((sk = d2i_ASN1_SEQUENCE_ANY(NULL, 
                                  (const unsigned char **)&aBitStr.data,
                                  aBitStr.length)) <= 0) {
    PKI_ERROR(PKI_ERR_GENERAL, "Cannot decode the composite signature.");
    return 0;
  }

  // Debugging
  PKI_DEBUG("Signature Sequence is Unpacked (Num: %d)!", sk_ASN1_TYPE_num(sk));

  // Checks we have the right number of components
  if (sk_ASN1_TYPE_num(sk) != comp_key_num) {
    PKI_ERROR(PKI_ERR_SIGNATURE_VERIFY, 
      "Wrong number of signature's components (%d instead of %d)",
      sk_ASN1_TYPE_num(sk), comp_key_num);
    goto err;
  }

  // Process the internal components
  for (int i = 0; i < sk_ASN1_TYPE_num(sk); i++) {

    // Gets the single values
    if ((aType = sk_ASN1_TYPE_value(sk, i)) == NULL) {
      PKI_DEBUG("Cannot get the ASN1_TYPE for signature #%d", i);
      return 0;
    }

    // Checks we got the right type
    if ((aType->type != V_ASN1_BIT_STRING) || (aType->value.sequence == NULL)) {
      PKI_DEBUG("Decoding error on signature component #%d (type: %d, value: %p)", 
        i, aType->type, aType->value.sequence);
      return 0;
    }

    // Retrieves the i-th component
    if ((evp_pkey = COMPOSITE_KEY_get0(comp_key, i)) == NULL) {
      PKI_ERROR(PKI_ERR_MEMORY_ALLOC, "Cannot get %d-th component from Key", i);
      goto err;
    }

    // Let's build a PKEY CTX and assign it to the MD CTX
    pkey_ctx = EVP_PKEY_CTX_new(evp_pkey, NULL);
    if (!pkey_ctx) {
      PKI_ERROR(PKI_ERR_MEMORY_ALLOC, "Cannot allocate the %d PKEY CTX component", i);
      goto err;
    }

    // Initializes the Verify operation
    ret_code = EVP_PKEY_verify_init(pkey_ctx);
    if (ret_code != 1) {
      PKI_ERROR(PKI_ERR_SIGNATURE_CREATE, 
        "Cannot initialize %d component signature (EVP_PKEY_verify_init code %d)", 
        i, ret_code);
      goto err;
    }

    // Verifies the individual signature
    ret_code = EVP_PKEY_verify(pkey_ctx, 
                               aType->value.sequence->data,
                               (size_t)aType->value.sequence->length,
                               tbs,
                               (size_t)tbslen);
    
    // Checks the results of the verify
    if (ret_code != 1) {
      PKI_ERROR(PKI_ERR_SIGNATURE_CREATE, 
        "Cannot initialize signature for %d component (EVP_PKEY_sign code is %d)", 
        i, ret_code);
      goto err;
    }

    // Removes the reference to the pkey. This is needed
    // because the EVP_PKEY_CTX_free() will otherwise
    // try to free the memory of the pkey
    pkey_ctx->pkey = NULL;

    // Free the EVP_PKEY_CTX
    if (pkey_ctx) EVP_PKEY_CTX_free(pkey_ctx);
    pkey_ctx = NULL; // Safety

  }

  PKI_DEBUG("PMETH: Freeing the stack memory");

  // Free the stack memory
  while ((aType = sk_ASN1_TYPE_pop(sk)) != NULL) {
    ASN1_TYPE_free(aType);
  } sk_ASN1_TYPE_free(sk);
  sk = NULL; // Safety

  // Debugging
  PKI_DEBUG("PMETH Verify Ok!");

  // All Done.
  return 1;

err:
  // Free the stack memory
  if (sk != NULL) {
    while ((aType = sk_ASN1_TYPE_pop(sk)) != NULL) {
      ASN1_TYPE_free(aType);
    } sk_ASN1_TYPE_free(sk);
    sk = NULL; // Safety
  }

  // Free other memory objects
  if (pkey_ctx) EVP_PKEY_CTX_free(pkey_ctx);
  if (evp_pkey) EVP_PKEY_free(evp_pkey);

  // Error
  return 0;
}

// // Not Implemented
// static int verify_recover_init(EVP_PKEY_CTX *ctx) {
//   PKI_DEBUG("Not implemented, yet.");
//   return 0;
// }

// // Not Implemented
// static int verify_recover(EVP_PKEY_CTX        * ctx,
//                           unsigned char       * rout,
//                           size_t              * routlen,
//                           const unsigned char * sig,
//                           size_t                siglen) {
//   PKI_DEBUG("Not implemented, yet.");
//   return 0;
// }

// Implemented
// static int signctx_init(EVP_PKEY_CTX *ctx, EVP_MD_CTX *mctx) {

//   return 1;


//   COMPOSITE_CTX * comp_ctx = ctx->data;
//     // Algorithm specific CTX

//   COMPOSITE_KEY * comp_key = EVP_PKEY_get0(ctx->pkey);
//     // Pointer to inner structure

//   // Input Checks
//   if (!ctx || !comp_ctx) return 0;

//   for (int i = 0; i < COMPOSITE_KEY_num(comp_key); i++) {

//     EVP_MD_CTX * md_ctx = NULL;
//       // Digest context

//     EVP_PKEY_CTX * pkey_ctx = NULL;
//       // Component specific CTX

//     EVP_PKEY * pkey = COMPOSITE_KEY_get0(comp_key, i);
//       // Component specific key

//     // Let' check we have the right data
//     if (!COMPOSITE_CTX_get0(comp_ctx, i, &pkey_ctx, &md_ctx)) {
//       DEBUG("ERROR: Cannot Retrieve CTX for component #%d", i);
//       return 0;
//     }

//     // Checks on the pointers
//     if (!pkey || !pkey_ctx) return 0;

//     if (mctx) {
      
//       if (!md_ctx && 
//           ((md_ctx = EVP_MD_CTX_new()) == NULL)) {
//         DEBUG("ERROR: Cannot Allocate the MD CTX for Component #%d", i);
//       }

//       // Initializes the EVP_MD (alias to EVP_MD_reset)
//       EVP_MD_CTX_init(md_ctx);

//       // Copy the MD to the specific component
//       if ((mctx->digest != NULL) && 
//           (EVP_MD_CTX_copy(md_ctx, mctx) <= 0)) {
//         // This is ok, it fails when the mctx->digest is NULL
//         DEBUG("ERROR: Cannot copy the MD CTX for Component #%d", i);
//         return 0;
//       }
//     }

//     if (pkey_ctx->pmeth->signctx_init != NULL &&
//         (pkey_ctx->pmeth->signctx_init(pkey_ctx, 
//                                        md_ctx) != 1)) {
//       DEBUG("ERROR: Cannot Initialize Signature for Component #%d", i);
//       return 0;
//     }
//   }

//   // All Components have been initialized
//   return 1;
// }

// Implemented
static int signctx (EVP_PKEY_CTX *ctx, unsigned char *sig, size_t *siglen, EVP_MD_CTX *mctx) {

  DEBUG("Not implemented, yet.");
  return 0;
}


// Implemented
static int verifyctx_init(EVP_PKEY_CTX *ctx, EVP_MD_CTX *mctx) {
  DEBUG("Not implemented, yet.");
  return 0;
}

// Implemented
static int verifyctx (EVP_PKEY_CTX *ctx, const unsigned char *sig, int siglen, EVP_MD_CTX *mctx) {
  DEBUG("Not implemented, yet.");
  return 0;
}

// Not Implemented
static int encrypt_init(EVP_PKEY_CTX *ctx) {
  DEBUG("Not implemented, yet.");
  return 0;
}

// Not Implemented
static int encrypt_pmeth(EVP_PKEY_CTX *ctx, unsigned char *out, size_t *outlen, const unsigned char *in, size_t inlen) {
  DEBUG("Not implemented, yet.");
  return 0;
}

// Not Implemented
static int decrypt_init(EVP_PKEY_CTX *ctx) {
  DEBUG("Not implemented, yet.");
  return 0;
}

// Not Implemented
static int decrypt(EVP_PKEY_CTX *ctx, unsigned char *out, size_t *outlen, const unsigned char *in, size_t inlen) {
  DEBUG("Not implemented, yet.");
  return 0;
}

// Not Implemented
static int derive_init(EVP_PKEY_CTX *ctx) {
  DEBUG("Not implemented, yet.");
  return 0;
}

// Not Implemented
static int derive(EVP_PKEY_CTX *ctx, unsigned char *key, size_t *keylen) {
  DEBUG("Not implemented, yet.");
  return 0;
}

// Implemented
static int ctrl(EVP_PKEY_CTX *ctx, int type, int key_id, void *value) {

  // NOTE: The passed ctx does not have the PKEY
  // associated with it. This means we cannot act
  // on the key

  COMPOSITE_CTX * comp_ctx = ctx ? ctx->data : NULL;
    // Pointer to the Composite CTX

  EVP_PKEY * pkey = ctx && ctx->pkey ? ctx->pkey : NULL;;
    // Pointer to the PKEY to add/del

  COMPOSITE_KEY * comp_pkey = pkey ? EVP_PKEY_get0(pkey) : NULL;
    // Pointer to the Composite Key

  // Input checks
  if (!comp_ctx || !comp_pkey) {
    PKI_ERROR(PKI_ERR_PARAM_NULL, "Missing CTX (%p) or PKEY (%p)", comp_ctx, comp_pkey);
    return 0;
  }

  // PKI_DEBUG("PKEY_CTRL: Setting (ctrl) (type = %d) (key_id = %d, value = %p)",
  //       type, key_id, value);

  switch (type) {

    // ===================
    // OpenSSL CTRL Values
    // ===================

    case EVP_PKEY_CTRL_GET_MD: {
      PKI_DEBUG("[ PMETH ] ======= EVP_PKEY_CTRL_GET_MD (Val: %p) ========== ", comp_ctx->md);
      *(const EVP_MD **)value = comp_ctx->md;
    } break;

    case EVP_PKEY_CTRL_MD: {

      // Here we need to allocate the digest for each key in the
      // stack (if not there, let's allocate the memory and
      // initialize it)

      // Input checks
      if (!value) {
        PKI_DEBUG("Missing 2nd parameter (value)");
        return 0;
      }

      // Sets the MD
      comp_ctx->md = value;

      // Debug
      PKI_DEBUG("Successfully set the MD for Composite Key");

      // All Done
      return 1;

    } break;


    case EVP_PKEY_OP_TYPE_SIG: {
      DEBUG("[ PMETH ] ======= EVP_PKEY_OP_TYPE_SIG ========== ");
      PKI_DEBUG("Got EVP sign operation - missing code, returning ok");
    } break;

    case EVP_PKEY_CTRL_PEER_KEY:
    case EVP_PKEY_CTRL_SET_DIGEST_SIZE:
    case EVP_PKEY_CTRL_SET_MAC_KEY:
    case EVP_PKEY_CTRL_SET_IV: {
      DEBUG("ERROR: Non Supported CTRL");
      return -2;
    } break;

    case EVP_PKEY_CTRL_DIGESTINIT: {
      return 1;
    } break;

    case EVP_PKEY_CTRL_PKCS7_ENCRYPT:
    case EVP_PKEY_CTRL_PKCS7_DECRYPT:
    case EVP_PKEY_CTRL_PKCS7_SIGN:
    case EVP_PKEY_CTRL_CMS_ENCRYPT:
    case EVP_PKEY_CTRL_CMS_DECRYPT:
    case EVP_PKEY_CTRL_CMS_SIGN:
    case EVP_PKEY_CTRL_CIPHER: {
      // DEBUGGING
      PKI_DEBUG("CTRL: type = %d, param_1 = %d, param_2 = %p", type, key_id, value);
      PKI_DEBUG("CTRL: No action taken for type = %d", type);
      // All Done
      return 1;
    } break;

    // =====================
    // COMPOSITE CTRL Values
    // =====================

    case EVP_PKEY_CTRL_COMPOSITE_PUSH: {
      // Adds the Key to the internal stack
      if (!COMPOSITE_KEY_push(comp_ctx->components, (EVP_PKEY *)value)) {
        PKI_ERROR(PKI_ERR_X509_KEYPAIR_GENERATION, "Cannot add component (type %d) to composite key", pkey->type);
        return 0;
      }
      // All Done
      return 1;
    } break;

    case EVP_PKEY_CTRL_COMPOSITE_ADD: {
      // Adds the Key to the internal stack
      if (!COMPOSITE_KEY_add(comp_ctx->components, (EVP_PKEY *)value, key_id)) {
        PKI_ERROR(PKI_ERR_X509_KEYPAIR_GENERATION, "Cannot add component (type %d) to composite key", pkey->type);
        return 0;
      }
      // All Done
      return 1;
    } break;

    case EVP_PKEY_CTRL_COMPOSITE_DEL: {
      // Checks we have the key_id component
      if (key_id <= 0 || key_id >= COMPOSITE_KEY_num(comp_ctx->components)) {
        PKI_ERROR(PKI_ERR_X509_KEYPAIR_SIZE, "Component %d does not exists (max is %d)", 
          key_id, COMPOSITE_KEY_num(comp_ctx->components));
        return 0;
      }
      // Delete the specific item from the stack
      COMPOSITE_KEY_del(comp_ctx->components, key_id);
      // All Done
      return 1;
    } break;

    case EVP_PKEY_CTRL_COMPOSITE_POP: {
      // Checks we have at least one component
      if (key_id <= 0 || key_id >= COMPOSITE_KEY_num(comp_ctx->components)) {
        PKI_ERROR(PKI_ERR_X509_KEYPAIR_SIZE, "Component %d does not exists (max is %d)", 
          key_id, COMPOSITE_KEY_num(comp_ctx->components));
        return 0;
      }
      // Pops a Key
      COMPOSITE_KEY_pop_free(comp_ctx->components);
      // All Done
      return 1;
    } break;

    case EVP_PKEY_CTRL_COMPOSITE_CLEAR: {
      // Clears all components from the key
      COMPOSITE_KEY_clear(comp_ctx->components);
      // All Done
      return 1;
    } break;

    default: {
      PKI_ERROR(PKI_ERR_GENERAL, "[PKEY METHOD] Unrecognized CTRL option [%d]", type);
      return 0;
    }
  }

  // Returns OK
  return 1;
}

// // Not Implemented
// static int ctrl_str(EVP_PKEY_CTX *ctx, const char *type, const char *value) {
//   DEBUG("Not implemented, yet.");
//   return 0;
// }

// ===================
// OpenSSL 1.1.x+ Only
// ===================

// // Implemented
// static int digestsign(EVP_MD_CTX          * ctx,
//                       unsigned char       * sig,
//                       size_t              * siglen,
//                       const unsigned char * tbs,
//                       size_t                tbslen) {
  
//   PKI_DEBUG("Not Implemented, yet.");

//   // return sign(ctx, sig, siglen, tbs, tbslen);

//   return 0;
//   /*
//   COMPOSITE_KEY * comp_key = EVP_PKEY_get0(ctx && ctx->pkey ? ctx->pkey : NULL);
//     // Pointer to inner key structure

//   COMPOSITE_CTX * comp_ctx = ctx->data;
//     // Pointer to algorithm specific CTX

//   const int signature_size = EVP_PKEY_size(ctx->pkey);
//     // The total signature size

//   STACK_OF(ASN1_TYPE) *sk = NULL;
//     // Stack of ASN1_OCTET_STRINGs

//   ASN1_OCTET_STRING * oct_string = NULL;
//     // Output Signature to be added
//     // to the stack of signatures

//   ASN1_TYPE * aType = NULL;
//     // ASN1 generic wrapper

//   int comp_key_num = 0;
//     // Number of components

//   unsigned char * buff = NULL;
//   unsigned char * pnt  = NULL;
//   int buff_len =  0;
//     // Temp Pointers

//   int total_size = 0;
//     // Total Signature Size

//   if ((comp_key == NULL) || 
//       ((comp_key_num = COMPOSITE_KEY_num(comp_key)) <= 0)) {
//     DEBUG("ERROR: Cannot get the Composite key inner structure");
//     return 0;
//   }

//   if (sig == NULL) {
//     *siglen = (size_t)signature_size;
//     return 1;
//   }

//   if ((size_t)signature_size > (*siglen)) {
//     DEBUG("ERROR: Buffer is too small");
//     return 0;
//   }

//   if ((sk = sk_ASN1_TYPE_new_null()) == NULL) {
//     DEBUG("ERROR: Memory Allocation");
//     return 0;
//   }

//   for (int i = 0; i < comp_key_num; i++) {

//     EVP_PKEY_CTX * pkey_ctx = NULL;

//     EVP_MD_CTX * md_ctx = NULL;

//     if (!COMPOSITE_CTX_get0(comp_ctx, i, &pkey_ctx, &md_ctx)) {
//       DEBUG("ERROR: Cannot get %d-th component from CTX", i);
//       return 0;
//     }

//     DEBUG("Determining Signature Size for Component #%d", i);

//     // Let's get the size of the single signature
//     if (EVP_PKEY_sign(pkey_ctx, NULL, (size_t *)&buff_len, tbs, tbslen) != 1) {
//       DEBUG("ERROR: Null Size reported from Key Component #%d", i);
//       goto err;
//     }

//     // Allocate the buffer for the single signature
//     if ((pnt = buff = OPENSSL_malloc(buff_len)) == NULL) {
//       DEBUG("ERROR: Memory Allocation");
//       goto err;
//     }

//     DEBUG("PNT = %p, BUFF = %p", pnt, buff);

//     // Generates the single signature
//     if (EVP_PKEY_sign(pkey_ctx, pnt, (size_t *)&buff_len, tbs, tbslen) != 1) {
//       DEBUG("ERROR: Component #%d cannot generate signatures", i);
//       goto err;
//     }

//     DEBUG("PNT = %p, BUFF = %p", pnt, buff);

//     // Updates the overall real size
//     total_size += buff_len;

//     DEBUG("Generated Signature for Component #%d Successfully (size: %d)", i, buff_len);
//     DEBUG("Signature Total Size [So Far] ... %d", total_size);

//     if ((oct_string = ASN1_OCTET_STRING_new()) == NULL) {
//       DEBUG("ERROR: Memory Allocation");
//       goto err;
//     }

//     // This sets the internal pointers
//     ASN1_STRING_set0(oct_string, buff, buff_len);

//     // Resets the pointer and length after ownership transfer
//     buff = NULL; buff_len = 0;

//     // Let's now generate the ASN1_TYPE and add it to the stack
//     if ((aType = ASN1_TYPE_new()) == NULL) {
//       DEBUG("ERROR: Memory Allocation");
//       goto err;
//     }

//     // Transfer Ownership to the aType structure
//     ASN1_TYPE_set(aType, V_ASN1_OCTET_STRING, oct_string);
//     oct_string = NULL;

//     // Adds the component to the stack
//     if (!sk_ASN1_TYPE_push(sk, aType)) {
//       DEBUG("ERROR: Cannot push the new Type");
//       goto err;
//     }

//     // Transfers ownership
//     aType = NULL;
//   }

//   if ((buff_len = i2d_ASN1_SEQUENCE_ANY(sk, &buff)) <= 0) {
//     DEBUG("ERROR: Cannot ASN1 encode the Overall Composite Key");
//     goto err;
//   }

//   // Reporting the total size
//   DEBUG("Total Signature Size: %d (reported: %d)", total_size, EVP_PKEY_size(ctx->pkey))

//   // Free the stack's memory
//   while ((aType = sk_ASN1_TYPE_pop(sk)) == NULL) {
//     ASN1_TYPE_free(aType);
//   }
//   sk_ASN1_TYPE_free(sk);
//   sk = NULL;

//   // Sets the output buffer
//   sig = buff;
//   *siglen = buff_len;

//   // All Done
//   return 1;

// err:

//   DEBUG("ERROR: Signing failed");

//   // Here we need to cleanup the memory

//   return 0;
//   */
// }

// // Implemented
// static int digestverify(EVP_MD_CTX *ctx, const unsigned char *sig, size_t siglen, const unsigned char *tbs, size_t tbslen) {
//   PKI_DEBUG("Not implemented, yet.");
//   return 0;
// }

// // Not Implemented
// static int check(EVP_PKEY *pkey) {
//   PKI_DEBUG("Not implemented, yet.");
//   return 0;
// }

// // Not Implemented
// static int public_check(EVP_PKEY *pkey) {
//   PKI_DEBUG("Not implemented, yet.");
//   return 0;
// }

// // Not Implemented
// static int param_check(EVP_PKEY *pkey) {
//   PKI_DEBUG("Not implemented, yet.");
//   return 0;
// }

// // Not Implemented
// static int digest_custom(EVP_PKEY_CTX *ctx, EVP_MD_CTX *mctx) {
//   PKI_DEBUG("Not implemented, yet. Returning Ok anyway.");
//   return 1;
// }

// ======================
// PKEY Method Definition
// ======================
//
// The Definition of the EVP_PKEY_METHOD is a typedef
// of the evp_pkey_method_st from:
// - OPENSSL_SRC/crypto/evp/evp_locl.h (OPENSSL_VERSION <= 1.1.0 or prior)
// - OPENSSL_SRC/crypto/include/internal/evp_int.h (OPENSSL_VERSION >= 1.1.X+)

// NOTE: When the EVP_PKEY_FLAG_SIGCTX_CUSTOM is used, then we can
// return a NULL as a default MD, otherwise OpenSSL will stop the
// execution (see the do_sigver_init() at m_sigver.c:25) because
// it gets the default digest nid (EVP_PKEY_get_default_digest_nid())
// and if tht returns NULL, it assumes it has no valid default
// and returns an error (NO_DEFAULT_DIGEST).

// NOTE: The EVP_PKEY_FLAG_SIGCTX_CUSTOM, when you do not implement
// the signctx_ and verifyctx_ functions, has the side effect to not
// initialize the EVP_MD_CTX * that is passed via the EVP_DigestSign
// interface.

EVP_PKEY_METHOD composite_pkey_meth = {
    0,              // int pkey_id; // EVP_PKEY_COMPOSITE
    0,              // int flags; //EVP_PKEY_FLAG_SIGCTX_CUSTOM
    init,           // int (*init)(EVP_PKEY_CTX *ctx);
    0, // copy,     // int (*copy)(EVP_PKEY_CTX *dst, EVP_PKEY_CTX *src);
    cleanup,        // void (*cleanup)(EVP_PKEY_CTX *ctx);
    0,              // paramgen_init,  // int (*paramgen_init)(EVP_PKEY_CTX *ctx);
    0,              // paramgen,       // int (*paramgen)(EVP_PKEY_CTX *ctx, EVP_PKEY *pkey);
    0, // keygen_init, // int (*keygen_init)(EVP_PKEY_CTX *ctx);
    keygen,         // int (*keygen)(EVP_PKEY_CTX *ctx, EVP_PKEY *pkey);
    0, // sign_init,      // int (*sign_init) (EVP_PKEY_CTX *ctx);
    sign,           // int (*sign) (EVP_PKEY_CTX *ctx, unsigned char *sig, size_t *siglen, const unsigned char *tbs, size_t tbslen);
    0,              // verify_init,    // int (*verify_init) (EVP_PKEY_CTX *ctx);
    verify,         // int (*verify) (EVP_PKEY_CTX *ctx, const unsigned char *sig, size_t siglen, const unsigned char *tbs, size_t tbslen);
    0,              // verify_recover_init,  // int (*verify_recover_init) (EVP_PKEY_CTX *ctx);
    0,              // verify_recover, // int (*verify_recover) (EVP_PKEY_CTX *ctx, unsigned char *rout, size_t *routlen, const unsigned char *sig, size_t siglen);
    0,              // signctx_init,   // int (*signctx_init) (EVP_PKEY_CTX *ctx, EVP_MD_CTX *mctx);
    0,              // signctx,        // int (*signctx) (EVP_PKEY_CTX *ctx, unsigned char *sig, size_t *siglen, EVP_MD_CTX *mctx);
    0,              // verifyctx_init, // int (*verifyctx_init) (EVP_PKEY_CTX *ctx, EVP_MD_CTX *mctx);
    0,              // verifyctx,      // int (*verifyctx) (EVP_PKEY_CTX *ctx, const unsigned char *sig, int siglen, EVP_MD_CTX *mctx);
    0,              // encrypt_init,   // int (*encrypt_init) (EVP_PKEY_CTX *ctx);
    0,              // encrypt,        // int (*encrypt) (EVP_PKEY_CTX *ctx, unsigned char *out, size_t *outlen, const unsigned char *in, size_t inlen);
    0,              // decrypt_init,   // int (*decrypt_init) (EVP_PKEY_CTX *ctx);
    0,              // decrypt,        // int (*decrypt) (EVP_PKEY_CTX *ctx, unsigned char *out, size_t *outlen, const unsigned char *in, size_t inlen);
    0,              // derive_init,    // int (*derive_init) (EVP_PKEY_CTX *ctx);
    0,              // derive,         // int (*derive) (EVP_PKEY_CTX *ctx, unsigned char *key, size_t *keylen);
    ctrl,           // int (*ctrl) (EVP_PKEY_CTX *ctx, int type, int p1, void *p2);
    0,              // int (*ctrl_str) (EVP_PKEY_CTX *ctx, const char *type, const char *value);
#if (OPENSSL_VERSION_NUMBER >= 0x10100000L)
    // These are only available on OpenSSL v1.1.X+ //
    0, // digestsign,     // int (*digestsign) (EVP_MD_CTX *ctx, unsigned char *sig, size_t *siglen, const unsigned char *tbs, size_t tbslen);
    0, // digestverify,   // int (*digestverify) (EVP_MD_CTX *ctx, const unsigned char *sig, size_t siglen, const unsigned char *tbs, size_t tbslen);
    0,                    // int (*check) (EVP_PKEY *pkey);
    0,                    // int (*public_check) (EVP_PKEY *pkey);
    0, // param_check,    // int (*param_check) (EVP_PKEY *pkey);
    0, // digest_custom   // int (*digest_custom) (EVP_PKEY_CTX *ctx, EVP_MD_CTX *mctx);
#endif
};

#endif // ENABLE_COMPOSITE

/* END: composite_pmeth.c */
