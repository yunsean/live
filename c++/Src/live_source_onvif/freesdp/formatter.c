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
 * @file formatter.c
 *
 * @short Formatting module implementation.
 **/
#include <stdio.h>
#include "formatterpriv.h"

const char *
bandwidth_modifier_strings[] =
{
    "",
    "",
    "CT",
    "AS",
    "RS",
    "RR"
};

const char *
network_type_strings[] =
{
    "",
    "IN"
};

const char *
address_type_strings[] =
{
    "",
    "IP4",
    "IP6"
};

const char *
bw_mod_type_strings[] =
{
    "",
    "CT",
    "AS",
    "RS",
    "RR"
};

const char *
encryption_method_strings[] =
{
    "",
    "clear",
    "base64",
    "uri"
};

const char *
sendrecv_mode_strings[] =
{
    "",
    "sendrecv",
    "recvonly",
    "sendonly",
    "inactive"
};

const char *session_type_strings[] =
{
    "",
    "broadcast",
    "meeting",
    "moderated",
    "test",
    "H332"
};

const char *media_strings[] =
{
    "",
    "audio",
    "video",
    "text",
    "application",
    "data",
    "control"
};

const char *tp_strings[] =
{
    "",
    "RTP/AVP",
    "RTP/SAVP",
    "RTP/AVPF",
    "RTP/SAVPF",
    "udp",
    "TCP",
    "UDPTL",
    "vat",
    "rtp",
    "H.320"
};

const char *orient_strings[] =
{
    "",
    "portrait",
    "landscape",
    "seascape"
};

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
    time_t stop)
{
    fsdp_description_t *obj;

    if((NULL == dsc)
       || (NULL == session_name)
       || (NULL == session_id)
       || (NULL == announcement_version)
       || (NULL == owner_username)
       || (NULL == owner_address))
    {
        return FSDPE_INVALID_PARAMETER;
    }

    *dsc = fsdp_description_new();
    obj = *dsc;

    /* TODO: add sanity checks */
    obj->version = sdp_version;
    obj->o_username = strdup(owner_username);
    obj->o_session_id = strdup(session_id);
    obj->o_announcement_version = strdup(announcement_version);
    obj->o_network_type = owner_nt;
    obj->o_address_type = owner_at;
    obj->o_address = strdup(owner_address);
    obj->s_name = strdup(session_name);
    obj->time_periods = calloc(TIME_PERIODS_MAX_COUNT, sizeof(fsdp_time_period_t*));
    obj->time_periods[0] = calloc(1, sizeof(fsdp_time_period_t));
    obj->time_periods[0]->start = start;
    obj->time_periods[0]->stop = stop;
    obj->time_periods_count = 1;
    obj->time_periods[0]->repeats = calloc(REPEATS_MAX_COUNT, sizeof(fsdp_repeat_t*));
    obj->time_periods[0]->repeats_count = 0;

    /* TODO: *dsc->built = 1; */
    return FSDPE_OK;
}

fsdp_error_t fsdp_format(const fsdp_description_t *dsc, char **text_description)
{
    char *result;
    size_t len = 1024;

    if((NULL == dsc) || (NULL == text_description))
      return FSDPE_INVALID_PARAMETER;

    result = malloc(len);

    /* iterative calls to */
    fsdp_format_bounded(dsc, result, len);

    if(NULL != ((fsdp_description_t *)dsc)->format_result)
        free(((fsdp_description_t *)dsc)->format_result);
    ((fsdp_description_t *)dsc)->format_result = result;
    *text_description = result;

    return FSDPE_OK;
}

fsdp_error_t fsdp_format_bounded(const fsdp_description_t *dsc, char *text_description, size_t maxsize)
{
    unsigned int i = 0, len = 0;
    int chars_written = 0;

    if((NULL == dsc) || (NULL == text_description))
        return FSDPE_INVALID_PARAMETER;

    /* while ( len < maxsize ) */
    /* `v=' line (protocol version) */
    chars_written = snprintf(
        text_description,
        maxsize - len,
        "v=0\r\n");
    if(0 < chars_written)
    {
        len += chars_written;
    }

    /***************************************************************************/
    /* A) parse session-level description                                      */
    /***************************************************************************/
    /* o= line (owner/creator and session identifier). */
    len += snprintf(text_description + len, 100,
            "o=%s %s %s %2s %3s %s\r\n", dsc->o_username,
            dsc->o_session_id, dsc->o_announcement_version,
            network_type_strings[dsc->c_network_type],
            address_type_strings[dsc->o_address_type], dsc->o_address);

    /* s= line (session name). */
    len += snprintf(text_description + len, 100, "s=%s\r\n", dsc->s_name);

    /* `i=' line (session information) [optional] */
    if(dsc->i_information != NULL)
      len += snprintf(text_description + len, 100, "i=%s\r\n", dsc->i_information);

    /* `u=' line (URI of description) [optional] */
    if(dsc->u_uri != NULL)
      len += snprintf(text_description + len, 100, "u=%s\r\n", dsc->u_uri);

    /* `e=' lines (email address) [zero or more] */
    for(i = 0; i < dsc->emails_count; i++)
    {
        len += snprintf(text_description + len, 100, "e=%s\r\n", dsc->emails[i]);
    }

    /* `p=' lines (phone number) [zero or more] */
    for(i = 0; i < dsc->phones_count; i++)
    {
        len += snprintf(text_description + len, 100, "p=%s\r\n", dsc->phones[i]);
    }

    /* `c=' line (connection information - not required if included in all media) [optional] */
    if(FSDP_NETWORK_TYPE_UNDEFINED != dsc->c_network_type)
    {
        if(0 == dsc->c_address.address_ttl)
        {
            len += snprintf(text_description + len, 100, "c=%s %s %s\r\n",
                    network_type_strings[dsc->c_network_type],
                    address_type_strings[dsc->c_address_type],
                    dsc->c_address.address);
        }
        else
        {
            if(0 == dsc->c_address.address_count)
            {
                len += snprintf(text_description + len, 100, "c=%s %s %s/%d\r\n",
                        network_type_strings[dsc->c_network_type],
                        address_type_strings[dsc->c_address_type],
                        dsc->c_address.address, dsc->c_address.address_ttl);
            }
            else
            {
                len += snprintf(text_description + len, 100, "c=%s %s %s/%d/%d\r\n",
                        network_type_strings[dsc->c_network_type],
                        address_type_strings[dsc->c_address_type],
                        dsc->c_address.address, dsc->c_address.address_ttl,
                        dsc->c_address.address_count);
            }
        }
    }

    /* `b=' lines (bandwidth information) [optional] */
    for(i = 0; i < dsc->bw_modifiers_count; i++)
    {
        fsdp_bw_modifier_type_t mt = dsc->bw_modifiers[i].b_mod_type;
        if(FSDP_BW_MOD_TYPE_UNKNOWN == mt)
        {
            len += snprintf(text_description + len, 100, "b=%s:%lu\r\n",
                    dsc->bw_modifiers[i].b_unknown_bw_modt,
                    dsc->bw_modifiers[i].b_value);
        }
        else
        {
            len += snprintf(text_description + len, 100, "b=%s:%lu\r\n",
                    bw_mod_type_strings[mt], dsc->bw_modifiers[i].b_value);
        }
    }

    /* `t=' lines (time the session is active) [1 or more] */
    for(i = 0; i < dsc->time_periods_count; i++)
    {
        unsigned int j;
        len += snprintf(text_description + len, 100, "t=%lu %lu\r\n",
                dsc->time_periods[i]->start + NTP_EPOCH_OFFSET,
                dsc->time_periods[i]->stop + NTP_EPOCH_OFFSET);

        /* `r' lines [zero or more repeat times for each t=] */
        for(j = 0; j < dsc->time_periods[i]->repeats_count; j++)
        {
            fsdp_repeat_t *repeat = dsc->time_periods[i]->repeats[j];
            len += snprintf(text_description + len, 100, "r=%lu %lu %lu\r\n",
                    repeat->interval, repeat->duration,
                    repeat->offsets[0]); /* FIX offsets */
        }
    }

    /* `z=' line (time zone adjustments) [zero or more] */
    if(NULL != dsc->timezone_adj)
      len += snprintf(text_description + len, 100, "z=%s\r\n", dsc->timezone_adj);

    /* `k=' line (encryption key) [optional] */
    if(FSDP_ENCRYPTION_METHOD_UNDEFINED == dsc->k_encryption_method)
    {
        ;
    }
    else if(FSDP_ENCRYPTION_METHOD_PROMPT == dsc->k_encryption_method)
    {
        len += snprintf(text_description + len, 100, "k=prompt\r\n");
    }
    else
    {
        len += snprintf(text_description + len, 100, "k=%s:%s\r\n",
                encryption_method_strings[dsc->k_encryption_method],
                dsc->k_encryption_content);
    }

    /* `a=' lines (zero or more session attribute lines) [optional] */
    {
        unsigned int j;

        if(NULL != dsc->a_str_attributes[FSDP_SESSION_STR_ATT_CATEGORY])
          len += snprintf(text_description + len, 100, "a=cat:%s\r\n",
                  dsc->a_str_attributes[FSDP_SESSION_STR_ATT_CATEGORY]);

        if(NULL != dsc->a_str_attributes[FSDP_SESSION_STR_ATT_CATEGORY])
          len += snprintf(text_description + len, 100, "a=keywds:%s\r\n",
                  dsc->a_str_attributes[FSDP_SESSION_STR_ATT_CATEGORY]);

        if(NULL != dsc->a_str_attributes[FSDP_SESSION_STR_ATT_TOOL])
          len += snprintf(text_description + len, 100, "a=tool:%s\r\n",
                  dsc->a_str_attributes[FSDP_SESSION_STR_ATT_TOOL]);

        for(j = 0; j < dsc->a_rtpmaps_count; j++)
        {
            fsdp_rtpmap_t *rtpmap = dsc->a_rtpmaps[j];
            if(NULL == rtpmap->parameters)
          len += snprintf(text_description + len, 100,
                  "a=rtpmap:%s %s/%u\r\n",
                  rtpmap->pt, rtpmap->encoding_name, rtpmap->clock_rate);
            else
          len += snprintf(text_description + len, 100,
                  "a=rtpmap:%s %s/%u/%s\r\n",
                  rtpmap->pt, rtpmap->encoding_name, rtpmap->clock_rate,
                  rtpmap->parameters);
        }

        if(FSDP_SENDRECV_UNDEFINED != dsc->a_sendrecv_mode)
          len += snprintf(text_description + len, 100, "a=%s\r\n",
                  sendrecv_mode_strings[dsc->a_sendrecv_mode]);

        if(FSDP_SESSION_TYPE_UNDEFINED != dsc->a_type)
          len += snprintf(text_description + len, 100, "a=type:%s\r\n",
                  session_type_strings[dsc->a_type]);

        if(NULL != dsc->a_str_attributes[FSDP_SESSION_STR_ATT_CHARSET])
          len += snprintf(text_description + len, 100, "a=charset:%s\r\n",
                  dsc->a_str_attributes[FSDP_SESSION_STR_ATT_CHARSET]);

        for(j = 0; j < dsc->a_sdplangs_count; j++)
        {
            len += snprintf(text_description + len, 100, "a=sdplang:%s\r\n",
                    dsc->a_sdplangs[j]);
        }

        for(j = 0; j < dsc->a_langs_count; j++)
        {
            len += snprintf(text_description + len, 100, "a=lang:%s\r\n",
                    dsc->a_langs[j]);
        }

        /* attributes not recognized by FreeSDP */
        for(j = 0; j < dsc->unidentified_attributes_count; j++)
        {
            len += snprintf(text_description + len, 100, "a=%s\r\n",
                    dsc->unidentified_attributes[j]);
        }
    }

    /***************************************************************************/
    /* B) print media-level descriptions                                       */
    /***************************************************************************/
    for(i = 0; i < dsc->media_announcements_count; i++)
    {
        unsigned int j;
        fsdp_media_announcement_t *ann = dsc->media_announcements[i];
        {       /* `m=' line (media name, transport address and format list) */
            if(0 == ann->port_count)
          len += snprintf(text_description + len, 100, "m=%s %d %s",
                  media_strings[ann->media_type], ann->port,
                  tp_strings[ann->transport]);
            else
          len += snprintf(text_description + len, 100, "m=%s %d/%d %s",
                  media_strings[ann->media_type],
                  ann->port, ann->port_count,
                  tp_strings[ann->transport]);

            for(j = 0; j < ann->formats_count; j++)
          len += snprintf(text_description + len, 100, " %s", ann->formats[j]);
            len += snprintf(text_description + len, 100, "\r\n");
        }

        if(ann->i_title != NULL)
          len += snprintf(text_description + len, 100, "i=%s\r\n",
                  ann->i_title);

        if(FSDP_NETWORK_TYPE_UNDEFINED != ann->c_network_type)
        {
            if(0 == ann->c_address.address_ttl)
            {
                len += snprintf(text_description + len, 100, "c=%s %s %s\r\n",
                        network_type_strings[ann->c_network_type],
                        address_type_strings[ann->c_address_type],
                        ann->c_address.address);
            }
            else
            {
                if(0 == ann->c_address.address_count)
                {
                    len += snprintf(text_description + len, 100, "c=%s %s %s/%d\r\n",
                            network_type_strings[ann->c_network_type],
                            address_type_strings[dsc->c_address_type],
                            ann->c_address.address, ann->c_address.address_ttl);
                }
                else
                {
                    len += snprintf(text_description + len, 100, "c=%s %s %s/%d/%d\r\n",
                            network_type_strings[ann->c_network_type],
                            address_type_strings[ann->c_address_type],
                            ann->c_address.address, ann->c_address.address_ttl,
                            ann->c_address.address_count);
                }
            }
        }

        /* `b=' lines (bandwidth information) [optional] */
        for(j = 0; j < ann->bw_modifiers_count; j++)
        {
            fsdp_bw_modifier_type_t mt = ann->bw_modifiers[j].b_mod_type;
            if(FSDP_BW_MOD_TYPE_UNKNOWN == mt)
          len += snprintf(text_description + len, 100, "b=%s:%lu\r\n",
                  ann->bw_modifiers[j].b_unknown_bw_modt,
                  ann->bw_modifiers[j].b_value);
            else
          len += snprintf(text_description + len, 100, "b=%s:%lu\r\n",
                  bandwidth_modifier_strings[ann->bw_modifiers[j].b_mod_type],
                  ann->bw_modifiers[j].b_value);
        }

        /* `k=' line (encryption key) [optional] */
        if(FSDP_ENCRYPTION_METHOD_UNDEFINED == ann->k_encryption_method)
        {
            ;
        }
        else if(FSDP_ENCRYPTION_METHOD_PROMPT == ann->k_encryption_method)
        {
            len += snprintf(text_description + len, 100, "k=prompt\r\n");
        }
        else
        {
            len += snprintf(text_description + len, 100, "k=%s:%s\r\n",
                    encryption_method_strings[ann->k_encryption_method],
                    ann->k_encryption_content);
        }

        /* `a=' lines (zero or more media attribute lines) [optional] */
        if(0 != ann->a_ptime)
          len += snprintf(text_description + len, 100, "a=ptime:%lu\r\n",
                  ann->a_ptime);

        if(0 != ann->a_maxptime)
          len += snprintf(text_description + len, 100, "a=maxptime:%lu\r\n",
                  ann->a_maxptime);

        for(j = 0; j < ann->a_rtpmaps_count; j++)
        {
            fsdp_rtpmap_t *rtpmap = ann->a_rtpmaps[j];
            if(NULL == rtpmap->parameters)
          len += snprintf(text_description + len, 100,
                  "a=rtpmap:%s %s/%u\r\n",
                  rtpmap->pt, rtpmap->encoding_name, rtpmap->clock_rate);
            else
          len += snprintf(text_description + len, 100,
                  "a=rtpmap:%s %s/%u/%s\r\n",
                  rtpmap->pt, rtpmap->encoding_name, rtpmap->clock_rate,
                  rtpmap->parameters);
        }

        if(FSDP_ORIENT_UNDEFINED != ann->a_orient)
          len += snprintf(text_description + len, 100, "a=orient:%s\r\n",
                  orient_strings[ann->a_orient]);

        if(FSDP_SENDRECV_UNDEFINED != ann->a_sendrecv_mode)
          len += snprintf(text_description + len, 100, "a=%s\r\n",
                  sendrecv_mode_strings[ann->a_sendrecv_mode]);

        for(j = 0; j < ann->a_sdplangs_count; j++)
        {
            len += snprintf(text_description + len, 100, "a=sdplang:%s\r\n",
                    ann->a_sdplangs[j]);
        }

        for(j = 0; j < ann->a_langs_count; j++)
        {
            len += snprintf(text_description + len, 100, "a=lang:%s\r\n",
                    ann->a_langs[j]);
        }

        if(0.0 != ann->a_framerate)
          len += snprintf(text_description + len, 100, "a=framerate:%f\r\n",
                  ann->a_framerate);

        if(0 != ann->a_quality)
          len += snprintf(text_description + len, 100, "a=quality:%ul\r\n",
                  ann->a_quality);

        for(j = 0; j < ann->a_fmtps_count; j++)
        {
            len += snprintf(text_description + len, 100, "a=lang:%s\r\n",
                    ann->a_fmtps[j]);
        }

        if(NULL != ann->a_rtcp_address)
        {
            len += snprintf(text_description + len, 100, "a=rtcp:%u %s %s %s\r\n",
                    ann->a_rtcp_port,
                    network_type_strings[ann->a_rtcp_network_type],
                    address_type_strings[ann->a_rtcp_address_type],
                    ann->a_rtcp_address);
        }

        /* attributes not recognized by FreeSDP */
        for(j = 0; j < ann->unidentified_attributes_count; j++)
        {
            len += snprintf(text_description + len, 100, "a=%s\r\n",
                    ann->unidentified_attributes[j]);
        }
    }

    /* this should be a macro: */
    if((len + 5) > maxsize)
      return FSDPE_BUFFER_OVERFLOW;
    len += 5;

    /* strncpy */
    return FSDPE_INTERNAL_ERROR;
}


fsdp_error_t fsdp_set_information(fsdp_description_t *dsc, const char *info)
{
    if((NULL == dsc) || (NULL == info))
      return FSDPE_INVALID_PARAMETER;

    if(NULL != dsc->i_information)
      free(dsc->i_information);
    dsc->i_information = strdup(info);
    return FSDPE_OK;
}

fsdp_error_t fsdp_set_uri(fsdp_description_t *dsc, const char *uri)
{
    if((NULL == dsc) || (NULL == uri))
      return FSDPE_INVALID_PARAMETER;

    if(NULL != dsc->u_uri)
      free(dsc->u_uri);
    dsc->u_uri = strdup(uri);
    return FSDPE_OK;
}

fsdp_error_t fsdp_add_email(fsdp_description_t *dsc, char *email)
{
    if((NULL == dsc) || (NULL == email))
    {
        return FSDPE_INVALID_PARAMETER;
    }
    if(NULL == dsc->emails)
    {
        dsc->emails_count = 0;
        dsc->emails = calloc(EMAILS_MAX_COUNT, sizeof(char*));
    }
    if(dsc->emails_count < EMAILS_MAX_COUNT) /* FIX */
      dsc->emails[dsc->emails_count++] = strdup(email);

    return FSDPE_OK;
}

fsdp_error_t fsdp_add_phone(fsdp_description_t *dsc, char *phone)
{
    if((NULL == dsc) || (NULL == phone))
    {
        return FSDPE_INVALID_PARAMETER;
    }
    if(NULL == dsc->phones)
    {
        dsc->phones_count = 0;
        dsc->phones = calloc(PHONES_MAX_COUNT, sizeof(char*));
    }
    if(dsc->phones_count < PHONES_MAX_COUNT) /* FIX */
      dsc->phones[dsc->phones_count++] = strdup(phone);

    return FSDPE_OK;
}

fsdp_error_t fsdp_set_conn_address(fsdp_description_t *dsc,
                                   fsdp_network_type_t nt,
                                   fsdp_address_type_t at,
                                   const char *address,
                                   unsigned int address_ttl,
                                   unsigned int address_count)
{
    if((NULL == dsc) || (NULL == address))
    {
        return FSDPE_INVALID_PARAMETER;
    }
    dsc->c_network_type = nt;
    dsc->c_address_type = at;
    if(NULL != dsc->c_address.address)
      free(dsc->c_address.address);
    dsc->c_address.address = strdup(address);
    dsc->c_address.address_ttl = address_ttl;
    dsc->c_address.address_count = address_count;
    return FSDPE_OK;
}

fsdp_error_t fsdp_add_bw_info(fsdp_description_t *dsc,
                              fsdp_bw_modifier_type_t mt,
                              unsigned long int value,
                              const char *unk_bmt)
{
    if((NULL == dsc) || ((FSDP_BW_MOD_TYPE_UNKNOWN == mt) && (NULL == unk_bmt)))
    {
        return FSDPE_INVALID_PARAMETER;
    }
    if(NULL == dsc->bw_modifiers)
    {
        dsc->bw_modifiers_count = 0;
        dsc->bw_modifiers =
          calloc(BW_MODIFIERS_MAX_COUNT, sizeof(fsdp_bw_modifier_t));
    }
    if(dsc->bw_modifiers_count < BW_MODIFIERS_MAX_COUNT)
    { /* FIX */
        fsdp_bw_modifier_t *bwm = &(dsc->bw_modifiers[dsc->bw_modifiers_count]);
        bwm->b_mod_type = mt;
        bwm->b_value = value;
        if(FSDP_BW_MOD_TYPE_UNKNOWN == mt)
        {
            bwm->b_unknown_bw_modt = strdup(unk_bmt);
        }
        dsc->bw_modifiers_count++;
    }

    return FSDPE_OK;
}

fsdp_error_t fsdp_add_period(fsdp_description_t *dsc, time_t start, time_t stop)
{
    if((NULL == dsc))
      return FSDPE_INVALID_PARAMETER;

    if(dsc->time_periods_count < TIME_PERIODS_MAX_COUNT)
    {
        /* FIX */
        fsdp_time_period_t *period;
        dsc->time_periods[dsc->time_periods_count] =
          calloc(1, sizeof(fsdp_time_period_t));
        period = dsc->time_periods[dsc->time_periods_count];
        period->start = start;
        period->stop = stop;
        dsc->time_periods_count++;
    }
    return FSDPE_OK;
}

fsdp_error_t fsdp_add_repeat(fsdp_description_t *dsc,
                             unsigned long int interval,
                             unsigned long int duration,
                             const char *offsets)
{
    fsdp_time_period_t *period;

    if((NULL == dsc) || (NULL == offsets))
      return FSDPE_INVALID_PARAMETER;

    period = dsc->time_periods[dsc->time_periods_count];
    if(NULL == period->repeats)
    {
        period->repeats_count = 0;
        period->repeats =
          calloc(REPEATS_MAX_COUNT, sizeof(fsdp_repeat_t*));
    }
    if(period->repeats_count < REPEATS_MAX_COUNT)
    {
        /* FIX */
        fsdp_repeat_t *repeat;
        period->repeats[period->repeats_count] = calloc(1, sizeof(fsdp_repeat_t));
        repeat = period->repeats[period->repeats_count];
        repeat->interval = interval;
        repeat->duration = duration;
        /* TODO repeat->offsets = strdup(offsets); */
        repeat->offsets = NULL;
        period->repeats_count++;
    }
    return FSDPE_OK;
}

fsdp_error_t fsdp_set_encryption(fsdp_description_t *dsc, fsdp_encryption_method_t emethod, const char *ekey)
{
    if((NULL == dsc) || ((FSDP_ENCRYPTION_METHOD_PROMPT != emethod) && (NULL == ekey)))
    {
        return FSDPE_INVALID_PARAMETER;
    }
    dsc->k_encryption_method = emethod;
    if(NULL != dsc->k_encryption_content)
      free(dsc->k_encryption_content);
    if(FSDP_ENCRYPTION_METHOD_PROMPT != emethod)
      dsc->k_encryption_content = strdup(ekey);
    return FSDPE_OK;
}

fsdp_error_t fsdp_set_timezone_adj(fsdp_description_t *dsc, const char *adj)
{
    if((NULL == dsc) || (NULL == adj))
      return FSDPE_INVALID_PARAMETER;
    if(NULL != dsc->timezone_adj)
      free(dsc->timezone_adj);
    dsc->timezone_adj = strdup(adj);
    return FSDPE_OK;
}

fsdp_error_t fsdp_set_str_att(fsdp_description_t *dsc, fsdp_session_str_att_t att, const char *value)
{
    if(NULL == dsc)
      return FSDPE_INVALID_PARAMETER;
    if(att <= FSDP_LAST_SESSION_STR_ATT)
    {
        if(NULL != dsc->a_str_attributes[att])
          free(dsc->a_str_attributes[att]);
        dsc->a_str_attributes[att] = strdup(value);
    }
    else
    {
        return FSDPE_INVALID_PARAMETER;
    }
    return FSDPE_OK;
}

fsdp_error_t fsdp_add_sdplang(fsdp_description_t *dsc, const char* lang)
{
    if((NULL == dsc) || (NULL == lang))
      return FSDPE_INVALID_PARAMETER;

    if(NULL == dsc->a_sdplangs)
    {
        dsc->a_sdplangs_count = 0;
        dsc->a_sdplangs =
          calloc(SDPLANGS_MAX_COUNT, sizeof(char*));
    }
    if(dsc->a_sdplangs_count < SDPLANGS_MAX_COUNT)
    {
        dsc->a_sdplangs[dsc->a_sdplangs_count] = strdup(lang);
        dsc->a_sdplangs_count++;
    }
    return FSDPE_OK;
}

fsdp_error_t fsdp_add_lang(fsdp_description_t *dsc, const char* lang)
{
    if((NULL == dsc) || (NULL == lang))
      return FSDPE_INVALID_PARAMETER;

    if(NULL == dsc->a_langs)
    {
        dsc->a_langs_count = 0;
        dsc->a_langs = calloc(SDPLANGS_MAX_COUNT, sizeof(char*));
    }
    if(dsc->a_langs_count < SDPLANGS_MAX_COUNT)
    {
        dsc->a_langs[dsc->a_langs_count] = strdup(lang);
        dsc->a_langs_count++;
    }
    return FSDPE_OK;
}

fsdp_error_t fsdp_add_rtpmap(fsdp_description_t *dsc,
                             const char* payload_type,
                             const char *encoding_name,
                             unsigned int rate,
                             const char *parameters)
{
    if((NULL == dsc) || (NULL == encoding_name))
      return FSDPE_INVALID_PARAMETER;

    if(NULL == dsc->a_rtpmaps)
    {
        dsc->a_rtpmaps_count = 0;
        dsc->a_rtpmaps = calloc(MEDIA_RTPMAPS_MAX_COUNT, sizeof(fsdp_rtpmap_t*));
    }
    {
        unsigned int c = dsc->a_rtpmaps_count;
        if(c < MEDIA_RTPMAPS_MAX_COUNT)
        {
            dsc->a_rtpmaps[c] = calloc(1, sizeof(fsdp_rtpmap_t));
            dsc->a_rtpmaps[c]->pt = strdup(payload_type);
            dsc->a_rtpmaps[c]->encoding_name = strdup(encoding_name);
            dsc->a_rtpmaps[c]->clock_rate = rate;
            if(NULL != parameters)
            {
                dsc->a_rtpmaps[c]->parameters = strdup(parameters);
            }
            dsc->a_rtpmaps_count++;
        }
    }
    return FSDPE_OK;
}

fsdp_error_t fsdp_set_sendrecv(fsdp_description_t *dsc, fsdp_sendrecv_mode_t mode)
{
    if((NULL == dsc))
      return FSDPE_INVALID_PARAMETER;
    dsc->a_sendrecv_mode = mode;
    return FSDPE_OK;
}

fsdp_error_t fsdp_set_session_type(fsdp_description_t *dsc, fsdp_session_type_t type)
{
    if((NULL == dsc))
      return FSDPE_INVALID_PARAMETER;
    dsc->a_type = type;
    return FSDPE_OK;
}

fsdp_error_t fsdp_add_media(fsdp_description_t *dsc, fsdp_media_description_t *const mdsc)
{
    if((NULL == dsc) || (NULL == mdsc))
    {
        return FSDPE_INVALID_PARAMETER;
    }
    if(NULL == dsc->media_announcements)
    {
        dsc->media_announcements_count = 0;
        dsc->media_announcements = calloc(MEDIA_ANNOUNCEMENTS_MAX_COUNT, sizeof(fsdp_media_announcement_t*));
    }
    if(dsc->media_announcements_count < MEDIA_ANNOUNCEMENTS_MAX_COUNT)
    {
        dsc->media_announcements[dsc->media_announcements_count++] = mdsc; /* FIX */
    }
    return FSDPE_OK;
}

fsdp_error_t fsdp_make_media(fsdp_media_description_t **mdsc,
                             fsdp_media_t type,
                             unsigned int port,
                             unsigned int port_count,
                             fsdp_transport_protocol_t tp,
                             const char *format)
{
    fsdp_media_description_t *obj;

    if((NULL == mdsc) || (NULL == format))
      return FSDPE_INVALID_PARAMETER;

    *mdsc = calloc(1, sizeof(fsdp_media_description_t));
    obj = *mdsc;
    obj->media_type = type;
    obj->port = port;
    obj->port_count = port_count;
    obj->transport = tp;
    /* TODO: check that format is valid */
    obj->formats = calloc(MEDIA_FORMATS_MAX_COUNT, sizeof(char*));
    obj->formats[0] = strdup(format);
    obj->formats_count = 1;
    obj->a_rtpmaps = NULL;
    return FSDPE_OK;
}

fsdp_error_t fsdp_add_media_format(fsdp_media_description_t *mdsc, const char *format)
{
    if((NULL == mdsc) || (NULL == format))
    {
        return FSDPE_INVALID_PARAMETER;
    }
    if(mdsc->formats_count < MEDIA_FORMATS_MAX_COUNT)
    {
        mdsc->formats[mdsc->formats_count] = strdup(format);
        mdsc->formats_count++;
    }
    return FSDPE_OK;
}

fsdp_error_t fsdp_set_media_title(fsdp_media_description_t *mdsc, const char *title)
{
    if((NULL == mdsc) || (NULL == title))
    {
        return FSDPE_INVALID_PARAMETER;
    }
    if(NULL != mdsc->i_title)
    {
        free(mdsc->i_title);
    }
    mdsc->i_title = strdup(title);
    return FSDPE_OK;
}

fsdp_error_t fsdp_set_media_conn_address(fsdp_media_description_t *mdsc,
                                         fsdp_network_type_t nt,
                                         fsdp_address_type_t at,
                                         const char *address,
                                         unsigned int address_ttl,
                                         unsigned int address_count)
{
    if((NULL == mdsc) || (NULL == address))
    {
        return FSDPE_INVALID_PARAMETER;
    }
    mdsc->c_network_type = nt;
    mdsc->c_address_type = at;
    if(NULL != mdsc->c_address.address)
    {
        free(mdsc->c_address.address);
    }
    mdsc->c_address.address = strdup(address);
    mdsc->c_address.address_ttl = address_ttl;
    mdsc->c_address.address_count = address_count;
    return FSDPE_OK;
}

fsdp_error_t fsdp_add_media_bw_info(fsdp_media_description_t *mdsc,
                                    fsdp_bw_modifier_type_t mt,
                                    unsigned long int value,
                                    const char *unk_bmt)
{
    if((NULL == mdsc) || ((FSDP_BW_MOD_TYPE_UNKNOWN == mt) && (NULL == unk_bmt)))
    {
        return FSDPE_INVALID_PARAMETER;
    }
    if(NULL == mdsc->bw_modifiers)
    {
        mdsc->bw_modifiers_count = 0;
        mdsc->bw_modifiers = calloc(BW_MODIFIERS_MAX_COUNT, sizeof(char*));
    }
    if(mdsc->bw_modifiers_count < BW_MODIFIERS_MAX_COUNT)
    { /* FIX */
        fsdp_bw_modifier_t *bwm = &(mdsc->bw_modifiers[mdsc->bw_modifiers_count]);
        bwm->b_mod_type = mt;
        bwm->b_value = value;
        if(FSDP_BW_MOD_TYPE_UNKNOWN == mt)
        {
            bwm->b_unknown_bw_modt = strdup(unk_bmt);
        }
        mdsc->bw_modifiers_count++;
    }
    return FSDPE_OK;
}

fsdp_error_t fsdp_set_media_encryption(fsdp_media_description_t *mdsc,
                                       fsdp_encryption_method_t emethod,
                                       const char *ekey)
{
    if((NULL == mdsc) || ((FSDP_ENCRYPTION_METHOD_PROMPT != emethod) && (NULL == ekey)))
    {
        return FSDPE_INVALID_PARAMETER;
    }
    mdsc->k_encryption_method = emethod;
    if(NULL != mdsc->k_encryption_content)
    {
        free(mdsc->k_encryption_content);
    }
    if(FSDP_ENCRYPTION_METHOD_PROMPT != emethod)
    {
        mdsc->k_encryption_content = strdup(ekey);
    }
    return FSDPE_OK;
}

fsdp_error_t fsdp_set_media_ptime(fsdp_media_description_t *mdsc, unsigned int ptime)
{
    if((NULL == mdsc))
    {
        return FSDPE_INVALID_PARAMETER;
    }
    mdsc->a_ptime = ptime;
    return FSDPE_OK;
}

fsdp_error_t fsdp_set_media_maxptime(fsdp_media_description_t *mdsc, unsigned int maxptime)
{
    if((NULL == mdsc))
    {
        return FSDPE_INVALID_PARAMETER;
    }
    mdsc->a_maxptime = maxptime;
    return FSDPE_OK;
}


fsdp_error_t fsdp_add_media_fmtp(fsdp_media_description_t *mdsc, const char* fmtp)
{
    if((NULL == mdsc) || (NULL == fmtp))
    {
        return FSDPE_INVALID_PARAMETER;
    }

    if(NULL == mdsc->a_fmtps)
    {
        mdsc->a_fmtps_count = 0;
        mdsc->a_fmtps = calloc(SDPLANGS_MAX_COUNT, sizeof(char*));
    }
    if(mdsc->a_fmtps_count < SDPLANGS_MAX_COUNT)
    {
        mdsc->a_fmtps[mdsc->a_fmtps_count] = strdup(fmtp);
        mdsc->a_fmtps_count++;
    }
    return FSDPE_OK;
}

fsdp_error_t fsdp_set_media_orient(fsdp_media_description_t *mdsc, fsdp_orient_t orient)
{
    if((NULL == mdsc))
    {
        return FSDPE_INVALID_PARAMETER;
    }
    mdsc->a_orient = orient;
    return FSDPE_OK;
}

fsdp_error_t fsdp_add_media_sdplang(fsdp_media_description_t *mdsc, const char* lang)
{
    if((NULL == mdsc) || (NULL == lang))
    {
        return FSDPE_INVALID_PARAMETER;
    }

    if(NULL == mdsc->a_sdplangs)
    {
        mdsc->a_sdplangs_count = 0;
        mdsc->a_sdplangs = calloc(SDPLANGS_MAX_COUNT, sizeof(char*));
    }
    if(mdsc->a_sdplangs_count < SDPLANGS_MAX_COUNT)
    {
        mdsc->a_sdplangs[mdsc->a_sdplangs_count] = strdup(lang);
        mdsc->a_sdplangs_count++;
    }
    return FSDPE_OK;
}

fsdp_error_t fsdp_add_media_lang(fsdp_media_description_t *mdsc, const char* lang)
{
    if((NULL == mdsc) || (NULL == lang))
    {
        return FSDPE_INVALID_PARAMETER;
    }

    if(NULL == mdsc->a_langs)
    {
        mdsc->a_langs_count = 0;
        mdsc->a_langs = calloc(SDPLANGS_MAX_COUNT, sizeof(char*));
    }
    if(mdsc->a_langs_count < SDPLANGS_MAX_COUNT)
    {
        mdsc->a_langs[mdsc->a_langs_count] = strdup(lang);
        mdsc->a_langs_count++;
    }
    return FSDPE_OK;
}

fsdp_error_t fsdp_set_media_sendrecv(fsdp_media_description_t *mdsc, fsdp_sendrecv_mode_t mode)
{
    if((NULL == mdsc))
    {
        return FSDPE_INVALID_PARAMETER;
    }
    mdsc->a_sendrecv_mode = mode;
    return FSDPE_OK;
}

fsdp_error_t fsdp_set_media_framerate(fsdp_media_description_t *mdsc, float rate)
{
    if((NULL == mdsc))
    {
        return FSDPE_INVALID_PARAMETER;
    }
    mdsc->a_framerate = rate;
    return FSDPE_OK;
}

fsdp_error_t fsdp_set_media_quality(fsdp_media_description_t *mdsc, unsigned int q)
{
    if((NULL == mdsc))
    {
        return FSDPE_INVALID_PARAMETER;
    }
    mdsc->a_quality = q;
    return FSDPE_OK;
}

fsdp_error_t fsdp_add_media_rtpmap(fsdp_media_description_t *mdsc,
                                   const char* payload_type,
                                   const char *encoding_name,
                                   unsigned int rate,
                                   const char *parameters)
{
    if((NULL == mdsc) || (NULL == encoding_name))
    {
        return FSDPE_INVALID_PARAMETER;
    }

    if(NULL == mdsc->a_rtpmaps)
    {
        mdsc->a_rtpmaps_count = 0;
        mdsc->a_rtpmaps = calloc(MEDIA_RTPMAPS_MAX_COUNT, sizeof(fsdp_rtpmap_t*));
    }
    {
        unsigned int c = mdsc->a_rtpmaps_count;
        if(c < MEDIA_RTPMAPS_MAX_COUNT)
        {
            mdsc->a_rtpmaps[c] = calloc(1, sizeof(fsdp_rtpmap_t));
            mdsc->a_rtpmaps[c]->pt = strdup(payload_type);
            mdsc->a_rtpmaps[c]->encoding_name = strdup(encoding_name);
            mdsc->a_rtpmaps[c]->clock_rate = rate;
            if(NULL != parameters)
            {
                mdsc->a_rtpmaps[c]->parameters = strdup(parameters);
            }
            mdsc->a_rtpmaps_count++;
        }
    }
    return FSDPE_OK;
}

fsdp_error_t fsdp_set_media_rtcp(fsdp_media_description_t *mdsc,
                                 unsigned int port,
                                 fsdp_network_type_t nt,
                                 fsdp_address_type_t at,
                                 const char *address)
{
    if((NULL == mdsc) || (NULL == address))
    {
        return FSDPE_INVALID_PARAMETER;
    }
    mdsc->a_rtcp_port = port;
    mdsc->a_rtcp_network_type = nt;
    mdsc->a_rtcp_address_type = at;
    if(NULL != mdsc->a_rtcp_address)
    {
        free(mdsc->a_rtcp_address);
    }
    mdsc->a_rtcp_address = strdup(address);
    return FSDPE_OK;
}
