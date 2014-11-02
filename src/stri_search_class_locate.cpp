/* This file is part of the 'stringi' package for R.
 * Copyright (c) 2013-2014, Marek Gagolewski and Bartek Tartanus
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING,
 * BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


#include "stri_stringi.h"
#include "stri_container_utf8.h"
#include "stri_container_charclass.h"
#include "stri_container_logical.h"
#include <deque>
#include <utility>
using namespace std;


/**
 * Locate first or last occurrences of a character class in each string
 *
 * @param str character vector
 * @param pattern character vector
 * @return matrix with 2 columns
 *
 * @version 0.1-?? (Marek Gagolewski, 2013-06-04)
 *
 * @version 0.1-?? (Marek Gagolewski, 2013-06-15)
 *          Use StrContainerCharClass
 *
 * @version 0.1-?? (Marek Gagolewski, 2013-06-16)
 *          make StriException-friendly
 *
 * @version 0.2-1 (Marek Gagolewski, 2014-04-03)
 *          detects invalid UTF-8 byte stream
 *
 * @version 0.2-1 (Marek Gagolewski, 2014-04-05)
 *          StriContainerCharClass now relies on UnicodeSet
 */
SEXP stri__locate_firstlast_charclass(SEXP str, SEXP pattern, bool first)
{
   str = stri_prepare_arg_string(str, "str");
   pattern = stri_prepare_arg_string(pattern, "pattern");
   R_len_t vectorize_length =
      stri__recycling_rule(true, 2, LENGTH(str), LENGTH(pattern));

   STRI__ERROR_HANDLER_BEGIN
   StriContainerUTF8 str_cont(str, vectorize_length);
   StriContainerCharClass pattern_cont(pattern, vectorize_length);

   SEXP ret;
   STRI__PROTECT(ret = Rf_allocMatrix(INTSXP, vectorize_length, 2));
   stri__locate_set_dimnames_matrix(ret);
   int* ret_tab = INTEGER(ret);

   for (R_len_t i = pattern_cont.vectorize_init();
         i != pattern_cont.vectorize_end();
         i = pattern_cont.vectorize_next(i))
   {
      ret_tab[i]                  = NA_INTEGER;
      ret_tab[i+vectorize_length] = NA_INTEGER;

      if (str_cont.isNA(i) || pattern_cont.isNA(i))
         continue;

      const UnicodeSet* pattern_cur = &pattern_cont.get(i);
      R_len_t     str_cur_n = str_cont.get(i).length();
      const char* str_cur_s = str_cont.get(i).c_str();
      R_len_t j;
      R_len_t k = 0;
      UChar32 chr;

      for (j=0; j<str_cur_n; ) {
         U8_NEXT(str_cur_s, j, str_cur_n, chr);
         if (chr < 0) // invalid utf-8 sequence
            throw StriException(MSG__INVALID_UTF8);
         k++; // 1-based index
         if (pattern_cur->contains(chr)) {
            ret_tab[i]      = k;
            if (first) break; // that's enough for first
            // note that for last, we can't go backwards from the end, as we need a proper index!
         }
      }
      ret_tab[i+vectorize_length] = ret_tab[i];
   }

   STRI__UNPROTECT_ALL
   return ret;
   STRI__ERROR_HANDLER_END(;/* nothing special to be done on error */)
}


/**
 * Locate first occurrence of a character class in each string
 *
 * @param str character vector
 * @param pattern character vector
 * @return matrix with 2 columns
 *
 * @version 0.1-?? (Marek Gagolewski, 2013-06-04)
 */
SEXP stri_locate_first_charclass(SEXP str, SEXP pattern)
{
   return stri__locate_firstlast_charclass(str, pattern, true);
}


/**
 * Locate last occurrence of a character class in each string
 *
 * @param str character vector
 * @param pattern character vector
 * @return matrix with 2 columns
 *
 * @version 0.1-?? (Marek Gagolewski, 2013-06-04)
 */
SEXP stri_locate_last_charclass(SEXP str, SEXP pattern)
{
   return stri__locate_firstlast_charclass(str, pattern, false);
}


/**
 * Locate first or last occurrences of a character class in each string
 *
 * @param str character vector
 * @param pattern character vector
 * @return list of matrices with 2 columns
 *
 * @version 0.1-?? (Marek Gagolewski, 2013-06-04)
 *
 * @version 0.1-?? (Marek Gagolewski, 2013-06-09)
 *          use R_len_t_x2 for merge=TRUE
 *          [R_len_t_x2 changed to pair<R_len_t, R_len_t> thereafter]
 *
 * @version 0.1-?? (Marek Gagolewski, 2013-06-15)
 *          Use StrContainerCharClass
 *
 * @version 0.1-?? (Marek Gagolewski, 2013-06-16)
 *          make StriException-friendly
 *
 * @version 0.2-1 (Marek Gagolewski, 2014-04-03)
 *          detects invalid UTF-8 byte stream
 *
 * @version 0.2-1 (Marek Gagolewski, 2014-04-05)
 *          StriContainerCharClass now relies on UnicodeSet
 */
SEXP stri_locate_all_charclass(SEXP str, SEXP pattern, SEXP merge)
{
   str     = stri_prepare_arg_string(str, "str");
   pattern = stri_prepare_arg_string(pattern, "pattern");
   merge   = stri_prepare_arg_logical(merge, "merge");
   R_len_t vectorize_length = stri__recycling_rule(true, 3,
         LENGTH(str), LENGTH(pattern), LENGTH(merge));

   STRI__ERROR_HANDLER_BEGIN
   StriContainerUTF8 str_cont(str, vectorize_length);
   StriContainerCharClass pattern_cont(pattern, vectorize_length);
   StriContainerLogical merge_cont(merge, vectorize_length);

   SEXP notfound; // this matrix will be set iff not found or NA
   STRI__PROTECT(notfound = stri__matrix_NA_INTEGER(1, 2));

   SEXP ret;
   STRI__PROTECT(ret = Rf_allocVector(VECSXP, vectorize_length));

   for (R_len_t i = pattern_cont.vectorize_init();
         i != pattern_cont.vectorize_end();
         i = pattern_cont.vectorize_next(i))
   {
      if (pattern_cont.isNA(i) || str_cont.isNA(i) || merge_cont.isNA(i)) {
         SET_VECTOR_ELT(ret, i, notfound);
         continue;
      }

      bool merge_cur        = merge_cont.get(i);
      const UnicodeSet* pattern_cur = &pattern_cont.get(i);
      R_len_t     str_cur_n = str_cont.get(i).length();
      const char* str_cur_s = str_cont.get(i).c_str();
      R_len_t j;
      R_len_t k = 0;
      UChar32 chr;
      deque< R_len_t > occurrences; // codepoint based-indices

      for (j=0; j<str_cur_n; ) {
         U8_NEXT(str_cur_s, j, str_cur_n, chr);
         if (chr < 0) // invalid utf-8 sequence
            throw StriException(MSG__INVALID_UTF8);
         k++; // 1-based index
         if (pattern_cur->contains(chr)) {
            occurrences.push_back(k);
         }
      }

      R_len_t noccurrences = (R_len_t)occurrences.size();
      if (noccurrences == 0)
         SET_VECTOR_ELT(ret, i, notfound);
      else if (merge_cur && noccurrences > 1) {
         // do merge
         deque< pair<R_len_t, R_len_t> > occurrences2;
         deque<R_len_t>::iterator iter = occurrences.begin();
         occurrences2.push_back(pair<R_len_t, R_len_t>(*iter, *iter));
         for (++iter; iter != occurrences.end(); ++iter) {
            R_len_t curoccur = *iter;
            if (occurrences2.back().second == curoccur - 1) { // continue seq
               occurrences2.back().second = curoccur;  // change `end`
            }
            else { // new seq
               occurrences2.push_back(pair<R_len_t, R_len_t>(curoccur, curoccur));
            }
         }

         // create resulting matrix from occurrences2
         R_len_t noccurrences2 = (R_len_t)occurrences2.size();
         SEXP cur_res;
         STRI__PROTECT(cur_res = Rf_allocMatrix(INTSXP, noccurrences2, 2));
         int* cur_res_int = INTEGER(cur_res);
         deque< pair<R_len_t, R_len_t> >::iterator iter2 = occurrences2.begin();
         for (R_len_t f = 0; iter2 != occurrences2.end(); ++iter2, ++f) {
            pair<R_len_t, R_len_t> curoccur = *iter2;
            cur_res_int[f] = curoccur.first;
            cur_res_int[f+noccurrences2] = curoccur.second;
         }
         SET_VECTOR_ELT(ret, i, cur_res);
         STRI__UNPROTECT(1)
      }
      else {
         // do not merge
         SEXP cur_res;
         STRI__PROTECT(cur_res = Rf_allocMatrix(INTSXP, noccurrences, 2));
         int* cur_res_int = INTEGER(cur_res);
         deque<R_len_t>::iterator iter = occurrences.begin();
         for (R_len_t f = 0; iter != occurrences.end(); ++iter, ++f)
            cur_res_int[f] = cur_res_int[f+noccurrences] = *iter;
         SET_VECTOR_ELT(ret, i, cur_res);
         STRI__UNPROTECT(1)
      }
   }

   stri__locate_set_dimnames_list(ret);
   STRI__UNPROTECT_ALL
   return ret;
   STRI__ERROR_HANDLER_END(;/* nothing special to be done on error */)
}
