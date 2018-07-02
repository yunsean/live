/*
  This file is part of FreeSDP
  Copyright (C) 2001,2002,2003,2004 Federico Montesino Pouzols <fedemp@altern.org>

  FreeSDP is free software; you can redistribute it and/or modify it
  under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  FreeSDP is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

/**
 * @file formatter.h
 * @ingroup formatter
 * @short Specific public header for FreeSDP formatting module.
 **/

#ifndef FSDP_FORMATTER_H
#define FSDP_FORMATTER_H

#include <freesdp/common.h>

BEGIN_C_DECLS

/**
 * @defgroup formatter FreeSDP Formatting Module
 *
 * SDP descriptions formatting routines.
 * @{
 **/

/**
 * Build a minimal SDP description object with the properties
 * corresponding to the parameters provided.
 * 
 * @param dsc description object to build.
 * @param sdp_version SDP protocol version, should be zero.
 * @param session_name with session name string.
 * @param session_id unique number identifying this session.
 * @param announcement_version version number of announcement
 * @param owner_username login or username of session owner.
 * @param owner_nt network type of session owner.
 * @param owner_at address type of session owner.
 * @param owner_address network address of owner, should match owner_at.
 * @param start session start time in time_t units.
 * @param stop session stop time in time_t units.
 *
 * @return
 * @retval
 **/
fsdp_error_t
fsdp_make_description(
    fsdp_description_t **dsc,
    unsigned int sdp_version,
    const char *session_name,
    const char *session_id,
    const char *announcement_version,
    const char *owner_username,
    fsdp_network_type_t owner_nt,
    fsdp_address_type_t owner_at,
    const char *owner_address,
    time_t start,
    time_t stop);

/**
 * Store a multimedia session description formatted in SDP into
 * <code>description</code>. The properties must have been set in
 * <code>dsc</code>. You don't have to free text_description.
 *
 * @param dsc object from which session properties will be taken.
 * @param text_description a multimedia session description formatted in SDP.
 *
 * @return FSDPE_OK if the description could be successfully
 * formatted. Otherwise, another error code is returned.
 **/
fsdp_error_t
fsdp_format(const fsdp_description_t *dsc, char **text_description);

/**
 * @param dsc object from which session properties will be taken.
 * @param text_description a multimedia session description formatted in SDP.
 * @param maxsize maximun number of octets the description should take.
 *
 * @return FSDPE_OK if the description could be successfully
 * formatted. Otherwise, another error code is returned.
 **/
fsdp_error_t
fsdp_format_bounded(const fsdp_description_t *dsc, char *text_description,
		    size_t maxsize);

/**
 * Set the information line about the session in <code>dsc</code>.
 *
 * @param dsc SDP description object being built.
 * @param info information line of this session.
 * @return FSDPE_OK if successful
 **/
fsdp_error_t
fsdp_set_information(fsdp_description_t *dsc, const char *info);

/**
 * Set an URI about the session in <code>dsc</code>. An SDP
 * description carries only one URI field.
 *
 * @param dsc SDP description object being built.
 * @param uri string containing an URI.
 * @return FSDPE_OK if successful
 **/
fsdp_error_t
fsdp_set_uri(fsdp_description_t *dsc, const char *uri);

/**
 * Add a contact email for the session in <code>dsc</code>. As with
 * fsdp_add_uri(), this function can be called more than once with the
 * same fsdp_description_t object; in that case, several emails will
 * be included in the description --listed in the same order as they
 * were specified through this function.
 *
 * @param dsc SDP description object being built.
 * @param email string containing an email about the session.
 * @return FSDPE_OK if successful
 **/
fsdp_error_t
fsdp_add_email(fsdp_description_t *dsc, char *email);

/**
 * Add a contact phone for the session in <code>dsc</code>. As with
 * fsdp_add_email(), this function can be called more than once with
 * the same fsdp_description_t object; in that case, several phones
 * will be included in the description --listed in the same order as
 * they were specified through this function.
 *
 * @param dsc SDP description object being built.
 * @param phone string containing a phone number about the session.
 * @return FSDPE_OK if successful
 **/
fsdp_error_t
fsdp_add_phone(fsdp_description_t *dsc, char *phone);

/**
 * Set the the global network address (as well as the network type and
 * the address type) of the multimedia session connection. You can
 * omit calling this function if the network address is defined for
 * each media individually.
 *
 * @param dsc SDP description object being built.
 * @param nt global network type for this connection. 
 * @param at global network address type for this connection.
 * @param address global network address for this connection.
 * @param address_ttl TTL specifier if an IP4 multicast address.
 * @param address_count number of consecutive ports.
 * @return FSDPE_OK if successful
 **/
fsdp_error_t
fsdp_set_conn_address(fsdp_description_t *dsc, fsdp_network_type_t nt,
		      fsdp_address_type_t at, const char *address,
		      unsigned int address_ttl, unsigned int address_count);

/**
 * Add a global bandwidth modifier for the session. You can omit
 * calling this function if the bandwidth modifier is defined for each
 * media individually. For defined bandwidth modifiers (others than
 * FSDP_BW_MODT_UNDEFINED) , the third (<code>unk_bmt</code>)
 * parameter will be ignored, and should be set to NULL.
 *
 * @param dsc SDP description object being built.
 * @param mt global bandwidth modifier type. If FSDP_BW_MODT_UNDEFINED
 * is specified, the textual bandwidth modifier must be given in
 * <code>unk_bmt</code>
 * @param value textual global bandwidth value.
 * @param unk_bm NULL unless bm is FSDP_BW_MODT_UNDEFINED, in which case it
 * is ignored; otherwise a textual bandwidth modifier.
 * @return FSDPE_OK if successful
 **/
fsdp_error_t
fsdp_add_bw_info(fsdp_description_t *dsc,
		 fsdp_bw_modifier_type_t mt, unsigned long int value,
		 const char *unk_bm);

/**
 *
 **/
fsdp_error_t
fsdp_add_period(fsdp_description_t *dsc, time_t start, time_t stop);

/* TODO: clarify how to link periods and repeats */
fsdp_error_t
fsdp_add_repeat(fsdp_description_t *dsc, unsigned long int interval,
		unsigned long int duration, const char *offsets);

/**
 * Set the encryption method, and a key or a URI pointing to the
 * encryption key for this session. If method is
 * FSDP_ENCRYPTION_METHOD_PROMPT, ekey is ignored and not included in
 * the session description.
 *
 * @param dsc SDP description object being built.
 * @param emethod encryption method.
 * @param ekey encryption key as a string.
 * @return FSDPE_OK if successful.
 **/
fsdp_error_t
fsdp_set_encryption(fsdp_description_t *dsc, fsdp_encryption_method_t emethod,
		    const char *ekey);

/**
 *
 **/
fsdp_error_t
fsdp_set_timezone_adj(fsdp_description_t *dsc, const char *adj);

/**
 *
 **/
fsdp_error_t
fsdp_set_str_att(fsdp_description_t *dsc, fsdp_session_str_att_t att,
		 const char *value);

/**
 *
 **/
fsdp_error_t
fsdp_add_sdplang(fsdp_description_t *dsc, const char* lang);

/**
 *
 **/
fsdp_error_t
fsdp_add_lang(fsdp_description_t *dsc, const char* lang);

/**
 *
 **/
fsdp_error_t
fsdp_set_sendrecv(fsdp_description_t *dsc, fsdp_sendrecv_mode_t mode);

/**
 *
 **/
fsdp_error_t
fsdp_set_session_type(fsdp_description_t *dsc, fsdp_session_type_t type);

/**
 *
 **/
fsdp_error_t
fsdp_add_media(fsdp_description_t *dsc, fsdp_media_description_t *const mdsc);

/**
 *
 **/
fsdp_error_t
fsdp_make_media(fsdp_media_description_t **mdsc, fsdp_media_t type,
		unsigned int port, unsigned int port_count,
		fsdp_transport_protocol_t tp, const char *format);

/**
 *
 **/
fsdp_error_t
fsdp_add_media_format(fsdp_media_description_t *mdsc, const char *format);

/**
 *
 **/
fsdp_error_t
fsdp_set_media_title(fsdp_media_description_t *mdsc, const char *title);

/**
 *
 **/
fsdp_error_t
fsdp_set_media_conn_address(fsdp_media_description_t *mdsc,
			    fsdp_network_type_t nt, fsdp_address_type_t at, 
			    const char *address, unsigned int address_ttl, 
			    unsigned int address_count);

/**
 *
 **/
fsdp_error_t
fsdp_add_media_bw_info(fsdp_media_description_t *mdsc, 
		       fsdp_bw_modifier_type_t mt,
		       unsigned long int value, const char *unk_bm);

/**
 *
 **/
fsdp_error_t
fsdp_set_media_encryption(fsdp_media_description_t *mdsc,
			  fsdp_encryption_method_t em, const char *content);

/**
 *
 **/
fsdp_error_t
fsdp_set_media_ptime(fsdp_media_description_t *mdsc, unsigned int ptime);

/**
 *
 **/
fsdp_error_t
fsdp_set_media_maxptime(fsdp_media_description_t *mdsc, unsigned int maxptime);

/**
 *
 **/
fsdp_error_t
fsdp_add_media_fmtp(fsdp_media_description_t *mdsc, const char* fmtp);

/**
 *
 **/
fsdp_error_t
fsdp_set_media_orient(fsdp_media_description_t *mdsc, fsdp_orient_t orient);

/**
 *
 **/
fsdp_error_t
fsdp_add_media_sdplang(fsdp_media_description_t *mdsc, const char* lang);

/**
 *
 **/
fsdp_error_t
fsdp_add_media_lang(fsdp_media_description_t *mdsc, const char* lang);

/**
 *
 **/
fsdp_error_t
fsdp_set_media_sendrecv(fsdp_media_description_t *mdsc, 
			fsdp_sendrecv_mode_t mode);

/**
 *
 **/
fsdp_error_t
fsdp_set_media_framerate(fsdp_media_description_t *mdsc, float rate);

/**
 *
 **/
fsdp_error_t
fsdp_set_media_quality(fsdp_media_description_t *mdsc, unsigned int q);

/**
 *
 **/
fsdp_error_t
fsdp_add_media_rtpmap(fsdp_media_description_t *mdsc, const char* payload_type,
		      const char *encoding_name, unsigned int rate, 
		      const char *parameters);

/**
 *
 **/
fsdp_error_t
fsdp_set_media_rtcp(fsdp_media_description_t *mdsc, unsigned int port,
		    fsdp_network_type_t nt, fsdp_address_type_t at,
		    const char *address);

/** @} */ /* closes formatter group */

END_C_DECLS

#endif /* FSDP_FORMATTER_H */
