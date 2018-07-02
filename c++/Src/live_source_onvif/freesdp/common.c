/*
This file is part of FreeSDP
Copyright (C) 2001,2002,2003,2004 Federico Montesino Pouzols <fedemp@suidzer0.org>

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
* @file common.c
*
* @short Implementation of routines common to parse and formatting
* modules .
*
* This file implements the routines that operate over data structures
* that are used in both the parse and formatting modules.
**/

#include "priv.h"
#include <freesdp/common.h>

fsdp_description_t* fsdp_description_new(void)
{
    unsigned int i = 0;
    fsdp_description_t* result = malloc(sizeof(fsdp_description_t));
    if(NULL != result)
    {
        result->version = 0;
        result->o_username = NULL;
        result->o_session_id = NULL;
        result->o_announcement_version = NULL;
        result->o_network_type = FSDP_NETWORK_TYPE_UNDEFINED;
        result->o_address_type = FSDP_ADDRESS_TYPE_UNDEFINED;
        result->o_address = NULL;
        result->s_name = NULL;
        result->i_information = NULL;
        result->u_uri = NULL;
        result->emails = NULL;
        result->emails_count = 0;
        result->phones = NULL;
        result->phones_count = 0;
        /* At first, there is no session-level definition for these parameters */
        result->c_network_type = FSDP_NETWORK_TYPE_UNDEFINED;
        result->c_address_type = FSDP_ADDRESS_TYPE_UNDEFINED;
        result->c_address.address = NULL;
        result->c_address.address_count = 0;
        result->c_address.address_ttl = 0;
        /* there is no session-level definition for these parameters */
        result->bw_modifiers = NULL;
        result->bw_modifiers_count = 0;
        result->time_periods = NULL;
        result->time_periods_count = 0;
        result->timezone_adj = NULL;
        result->k_encryption_method = FSDP_ENCRYPTION_METHOD_UNDEFINED;
        result->k_encryption_content = NULL;
        /* Default/undefined values for attributes */
        for(i = 0; i < (FSDP_LAST_SESSION_STR_ATT + 1); i++)
        {
            result->a_str_attributes[i] = NULL;
        }
        result->a_rtpmaps = NULL;
        result->a_rtpmaps_count = 0;
        result->a_sendrecv_mode = FSDP_SENDRECV_UNDEFINED;
        result->a_type = FSDP_SESSION_TYPE_UNDEFINED;
        result->a_sdplangs = NULL;
        result->a_sdplangs_count = 0;
        result->a_langs = NULL;
        result->a_langs_count = 0;
        result->media_announcements = NULL;
        result->media_announcements_count = 0;
        result->unidentified_attributes = NULL;
        result->unidentified_attributes_count = 0;
        result->format_result = NULL;
    }
    return result;
}

void fsdp_description_delete(fsdp_description_t* dsc)
{
    fsdp_description_recycle(dsc);
    free(dsc);
}

void fsdp_description_recycle(fsdp_description_t* dsc)
{
    /* Recursively free all strings and arrays */
    unsigned int i, j;
    if(NULL == dsc)
    {
        return;
    }
    dsc->version = 0;
    free(dsc->o_username);
    dsc->o_username = NULL;
    free(dsc->o_session_id);
    dsc->o_session_id = NULL;
    free(dsc->o_announcement_version);
    dsc->o_announcement_version = NULL;
    dsc->o_network_type = FSDP_NETWORK_TYPE_UNDEFINED;
    dsc->o_address_type = FSDP_ADDRESS_TYPE_UNDEFINED;
    free(dsc->o_address);
    dsc->o_address = NULL;
    free(dsc->s_name);
    dsc->s_name = NULL;
    free(dsc->i_information);
    dsc->i_information = NULL;
    free(dsc->u_uri);
    dsc->u_uri = NULL;
    for(i = 0; i < dsc->emails_count; i++)
    {
        free((char*)dsc->emails[i]);
    }
    free(dsc->emails);
    dsc->emails = NULL;
    dsc->emails_count = 0;
    for(i = 0; i < dsc->phones_count; i++)
    {
        free((char*)dsc->phones[i]);
    }
    free(dsc->phones);
    dsc->phones = NULL;
    dsc->phones_count = 0;
    dsc->c_network_type = FSDP_NETWORK_TYPE_UNDEFINED;
    dsc->c_address_type = FSDP_ADDRESS_TYPE_UNDEFINED;
    free(dsc->c_address.address);
    dsc->c_address.address = NULL;
    dsc->c_address.address_count = 0;
    dsc->c_address.address_ttl = 0;
    for(i = 0; i < dsc->bw_modifiers_count; i++)
    {
        free(dsc->bw_modifiers[i].b_unknown_bw_modt);
    }
    free(dsc->bw_modifiers);
    dsc->bw_modifiers = NULL;
    dsc->bw_modifiers_count = 0;
    for(i = 0; i < dsc->time_periods_count; i++)
    {
        for(j = 0; j < dsc->time_periods[i]->repeats_count; j++)
        {
            free(dsc->time_periods[i]->repeats[j]->offsets);
            free(dsc->time_periods[i]->repeats[j]);
        }
        free(dsc->time_periods[i]->repeats);
        free(dsc->time_periods[i]);
    }
    free(dsc->time_periods);
    dsc->time_periods = NULL;
    dsc->time_periods_count = 0;
    free(dsc->timezone_adj);
    dsc->timezone_adj = NULL;
    dsc->k_encryption_method = FSDP_ENCRYPTION_METHOD_UNDEFINED;
    free(dsc->k_encryption_content);
    for(i = 0; i < (FSDP_LAST_SESSION_STR_ATT + 1); i++)
    {
        free(dsc->a_str_attributes[i]);
        dsc->a_str_attributes[i] = NULL;
    }
    for(i = 0; i < dsc->a_rtpmaps_count; i++)
    {
        free(dsc->a_rtpmaps[i]);
    }
    free(dsc->a_rtpmaps);
    dsc->a_rtpmaps = NULL;
    dsc->a_rtpmaps_count = 0;
    dsc->a_sendrecv_mode = FSDP_SENDRECV_UNDEFINED;
    dsc->a_type = FSDP_SESSION_TYPE_UNDEFINED;
    for(i = 0; i < dsc->a_sdplangs_count; i++)
    {
        free(dsc->a_sdplangs[i]);
    }
    free(dsc->a_sdplangs);
    dsc->a_sdplangs = NULL;
    dsc->a_sdplangs_count = 0;
    for(i = 0; i < dsc->a_langs_count; i++)
    {
        free(dsc->a_langs[i]);
    }
    free(dsc->a_langs);
    dsc->a_langs = NULL;
    dsc->a_langs_count = 0;
    for(i = 0; i < dsc->media_announcements_count; i++)
    {
        for(j = 0; j < dsc->media_announcements[i]->formats_count; j++)
        {
            free(dsc->media_announcements[i]->formats[j]);
        }
        free(dsc->media_announcements[i]->formats);
        free(dsc->media_announcements[i]->i_title);
        free(dsc->media_announcements[i]->c_address.address);
        for(j = 0; j < dsc->media_announcements[i]->bw_modifiers_count; j++)
        {
            if(FSDP_BW_MOD_TYPE_UNKNOWN == dsc->media_announcements[i]->bw_modifiers[j].b_mod_type)
            {
                free(dsc->media_announcements[i]->bw_modifiers[j].b_unknown_bw_modt);
            }
        }
        free(dsc->media_announcements[i]->bw_modifiers);
        free(dsc->media_announcements[i]->k_encryption_content);
        for(j = 0; j < dsc->media_announcements[i]->a_rtpmaps_count; j++)
        {
            free(dsc->media_announcements[i]->a_rtpmaps[j]->pt);
            free(dsc->media_announcements[i]->a_rtpmaps[j]->encoding_name);
            free(dsc->media_announcements[i]->a_rtpmaps[j]->parameters);
            free(dsc->media_announcements[i]->a_rtpmaps[j]);
        }
        free(dsc->media_announcements[i]->a_rtpmaps);
        for(j = 0; j < dsc->media_announcements[i]->a_sdplangs_count; j++)
        {
            free(dsc->media_announcements[i]->a_sdplangs[j]);
        }
        free(dsc->media_announcements[i]->a_sdplangs);
        for(j = 0; j < dsc->media_announcements[i]->a_langs_count; j++)
        {
            free(dsc->media_announcements[i]->a_langs[j]);
        }
        free(dsc->media_announcements[i]->a_langs);
        for(j = 0; j < dsc->media_announcements[i]->a_fmtps_count; j++)
        {
            free(dsc->media_announcements[i]->a_fmtps[j]);
        }
        free(dsc->media_announcements[i]->a_fmtps);
        free(dsc->media_announcements[i]->a_rtcp_address);
		free(dsc->media_announcements[i]->a_control);
        for(j = 0; j < dsc->media_announcements[i]->unidentified_attributes_count; j++)
        {
            free(dsc->media_announcements[i]->unidentified_attributes[j]);
        }
        free(dsc->media_announcements[i]->unidentified_attributes);
        free(dsc->media_announcements[i]);
    }
    free(dsc->media_announcements);
    dsc->media_announcements = NULL;
    dsc->media_announcements_count = 0;
    free(dsc->format_result);
    dsc->format_result = NULL;
}
