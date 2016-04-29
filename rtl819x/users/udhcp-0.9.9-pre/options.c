/* 
 * options.c -- DHCP server option packet tools 
 * Rewrite by Russ Dill <Russ.Dill@asu.edu> July 2001
 */
 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "debug.h"
#include "dhcpd.h"
#include "files.h"
#include "options.h"
#include "leases.h"

#ifdef AEI_TR111
#include "ctl.h"
#include "ctl_validstrings.h"
#include "libtr69_client.h"
#endif

#if defined(CONFIG_RTL865X_KLD)	
extern unsigned char update_lease_time1;
extern unsigned char update_option_dns;
#endif
/* supported options are easily added here */
struct dhcp_option options[] = {
	/* name[10]	flags					code */
	{"subnet",	OPTION_IP | OPTION_REQ,			0x01},
	{"timezone",	OPTION_S32,				0x02},
/* Aded 20080508 for option 33 */
#ifdef UDHCPC_STATIC_ROUTE
	{"fixroute",OPTION_IP | OPTION_REQ,			0x21},
#endif
/* Aded 20080508 for option 121 */
#ifdef RFC3442
#ifdef UDHCPC_RFC_CLASSLESS_STATIC_ROUTE
	{"rfc3442",OPTION_IP | OPTION_REQ,			0x79},
#endif
/* ----------------------------------- */
/* Aded 20080508 for option 249 */
#ifdef UDHCPC_MS_CLASSLESS_STATIC_ROUTE
	{"rfc3442",OPTION_IP | OPTION_REQ,			0xF9},
#endif
#endif
/* ----------------------------------- */
	{"router",	OPTION_IP | OPTION_LIST | OPTION_REQ,	0x03},
	{"timesvr",	OPTION_IP | OPTION_LIST,		0x04},
	{"namesvr",	OPTION_IP | OPTION_LIST,		0x05},
	{"dns",		OPTION_IP | OPTION_LIST | OPTION_REQ,	0x06},
	{"logsvr",	OPTION_IP | OPTION_LIST,		0x07},
	{"cookiesvr",	OPTION_IP | OPTION_LIST,		0x08},
	{"lprsvr",	OPTION_IP | OPTION_LIST,		0x09},
	{"hostname",	OPTION_STRING | OPTION_REQ,		0x0c},
	{"bootsize",	OPTION_U16,				0x0d},
	{"domain",	OPTION_STRING | OPTION_REQ,		0x0f},
	{"swapsvr",	OPTION_IP,				0x10},
	{"rootpath",	OPTION_STRING,				0x11},
	{"ipttl",	OPTION_U8,				0x17},
	{"mtu",		OPTION_U16,				0x1a},
	{"broadcast",	OPTION_IP | OPTION_REQ,			0x1c},
	{"ntpsrv",	OPTION_IP | OPTION_LIST,		0x2a},
	
	{"wins",	OPTION_IP | OPTION_LIST|OPTION_REQ,		0x2c},
	{"nbntype",	OPTION_U8 | OPTION_LIST|OPTION_REQ,		0x2e},
	{"nbscope",	OPTION_STRING | OPTION_LIST|OPTION_REQ,		0x2f},
	
	{"requestip",	OPTION_IP,				0x32},
	{"lease",	OPTION_U32,				0x33},
	{"dhcptype",	OPTION_U8,				0x35},
	{"serverid",	OPTION_IP,				0x36},
	{"message",	OPTION_STRING,				0x38},
	{"tftp",	OPTION_STRING,				0x42},
	{"bootfile",	OPTION_STRING,				0x43},

	{"",		0x00,				0x00}
};

/* Lengths of the different option types */
int option_lengths[] = {
	[OPTION_IP] =		4,
	[OPTION_IP_PAIR] =	8,
	[OPTION_BOOLEAN] =	1,
	[OPTION_STRING] =	1,
	[OPTION_U8] =		1,
	[OPTION_U16] =		2,
	[OPTION_S16] =		2,
	[OPTION_U32] =		4,
	[OPTION_S32] =		4
};


/* get an option with bounds checking (warning, not aligned). */
unsigned char *get_option(struct dhcpMessage *packet, int code)
{
	int i, length;
	unsigned char *optionptr=NULL;
	int over = 0, done = 0, curr = OPTION_FIELD;
	
	optionptr = packet->options;
	i = 0;
	length = 308;
	while (!done) {
		if (i >= length) {
			LOG(LOG_WARNING, "bogus packet, option fields too long.");
			return NULL;
		}
		if (optionptr[i + OPT_CODE] == code) {
			if (i + 1 + optionptr[i + OPT_LEN] >= length) {
				LOG(LOG_WARNING, "bogus packet, option fields too long.");
				return NULL;
			}
			return optionptr + i + 2;
		}			
		switch (optionptr[i + OPT_CODE]) {
		case DHCP_PADDING:
			i++;
			break;
		case DHCP_OPTION_OVER:
			if (i + 1 + optionptr[i + OPT_LEN] >= length) {
				LOG(LOG_WARNING, "bogus packet, option fields too long.");
				return NULL;
			}
			over = optionptr[i + 3];
			i += optionptr[OPT_LEN] + 2;
			break;
		case DHCP_END:
			if (curr == OPTION_FIELD && over & FILE_FIELD) {
				optionptr = packet->file;
				i = 0;
				length = 128;
				curr = FILE_FIELD;
			} else if (curr == FILE_FIELD && over & SNAME_FIELD) {
				optionptr = packet->sname;
				i = 0;
				length = 64;
				curr = SNAME_FIELD;
			} else done = 1;
			break;
		default:
			i += optionptr[OPT_LEN + i] + 2;
		}
	}
	return NULL;
}


/* return the position of the 'end' option (no bounds checking) */
int end_option(unsigned char *optionptr) 
{
	int i = 0;
	
	while (optionptr[i] != DHCP_END) {
		if (optionptr[i] == DHCP_PADDING) i++;
		else i += optionptr[i + OPT_LEN] + 2;
	}
	return i;
}


/* add an option string to the options (an option string contains an option code,
 * length, then data) */
int add_option_string(unsigned char *optionptr, unsigned char *string)
{
	int end = end_option(optionptr);
	
	/* end position + string length + option code/length + end option */
	if (end + string[OPT_LEN] + 2 + 1 >= 308) {
		LOG(LOG_ERR, "Option 0x%02x did not fit into the packet!", string[OPT_CODE]);
		return 0;
	}
	DEBUG(LOG_INFO, "adding option 0x%02x", string[OPT_CODE]);
	memcpy(optionptr + end, string, string[OPT_LEN] + 2);
	optionptr[end + string[OPT_LEN] + 2] = DHCP_END;
	return string[OPT_LEN] + 2;
}


/* add a one to four byte option to a packet */
int add_simple_option(unsigned char *optionptr, unsigned char code, u_int32_t data)
{
	char length = 0;
	int i;
	unsigned char option[2 + 4];
	unsigned char *u8;
	u_int16_t *u16;
	u_int32_t *u32;
	u_int32_t aligned;
	u8 = (unsigned char *) &aligned;
#ifdef AEI_WECB
	u16 = (u_int16_t *) u8;
#else
	u16 = (u_int16_t *) &aligned;
#endif

	u32 = &aligned;

	for (i = 0; options[i].code; i++)
		if (options[i].code == code) {
			length = option_lengths[options[i].flags & TYPE_MASK];
		}
		
	if (!length) {
		DEBUG(LOG_ERR, "Could not add option 0x%02x", code);
		return 0;
	}
	
	option[OPT_CODE] = code;
	option[OPT_LEN] = length;

	switch (length) {
		case 1: *u8 =  data; break;
		case 2: *u16 = data; break;
		case 4: *u32 = data; break;
	}
	memcpy(option + 2, &aligned, length);
	return add_option_string(optionptr, option);
}


/* find option 'code' in opt_list */
struct option_set *find_option(struct option_set *opt_list, char code)
{
	while (opt_list && opt_list->data[OPT_CODE] < code)
		opt_list = opt_list->next;

	if (opt_list && opt_list->data[OPT_CODE] == code) return opt_list;
	else return NULL;
}


/* add an option to the opt_list */
void attach_option(struct option_set **opt_list, struct dhcp_option *option, char *buffer, int length)
{
	struct option_set *existing, *new, **curr;

	/* add it to an existing option */
	if ((existing = find_option(*opt_list, option->code))) {
		DEBUG(LOG_INFO, "Attaching option %s to existing member of list", option->name);
#if defined(CONFIG_RTL865X_KLD)
		if (option->flags & OPTION_LIST) {
				if(!strcmp(option->name, "dns")){
					if(server_config.upateConfig_isp_dns == 1){
						if(update_lease_time1 ==1){
									if (existing->data[OPT_LEN] + length <= 255) {
												//printf("the orig length=%d\n", existing->data[OPT_LEN] );
												if(update_option_dns==0){
														existing->data = realloc(existing->data, length + 2);
														memcpy(existing->data+2, buffer, length);
														existing->data[OPT_LEN] = length;
														update_option_dns++;
													}else{
														existing->data = realloc(existing->data, existing->data[OPT_LEN] + length + 2);
														memcpy(existing->data + existing->data[OPT_LEN] + 2, buffer, length);
														existing->data[OPT_LEN] += length;
													}
									} 
							}
							#if 0
							else{
								if (existing->data[OPT_LEN] + length <= 255) {
										existing->data = realloc(existing->data, existing->data[OPT_LEN] + length + 2);
										memcpy(existing->data + existing->data[OPT_LEN] + 2, buffer, length);
										existing->data[OPT_LEN] += length;
								} /* else, ignore the data, we could put this in a second option in the future */
							}
							#endif
						} else {
								if (existing->data[OPT_LEN] + length <= 255) {
										existing->data = realloc(existing->data, existing->data[OPT_LEN] + length + 2);
										memcpy(existing->data + existing->data[OPT_LEN] + 2, buffer, length);
										existing->data[OPT_LEN] += length;
								} /* else, ignore the data, we could put this in a second option in the future */
						}
					} else {
						if (existing->data[OPT_LEN] + length <= 255) {
										existing->data = realloc(existing->data, existing->data[OPT_LEN] + length + 2);
										memcpy(existing->data + existing->data[OPT_LEN] + 2, buffer, length);
										existing->data[OPT_LEN] += length;
								} /* else, ignore the data, we could put this in a second option in the future */
					}
		}/* else, ignore the new data */
#else	
		if (option->flags & OPTION_LIST) {
			if (existing->data[OPT_LEN] + length <= 255) {
				existing->data = realloc(existing->data, existing->data[OPT_LEN] + length + 2);
				memcpy(existing->data + existing->data[OPT_LEN] + 2, buffer, length);
				existing->data[OPT_LEN] += length;
			} /* else, ignore the data, we could put this in a second option in the future */
		} /* else, ignore the new data */
#endif		
#if defined(CONFIG_RTL8186_TR)			
		else{
			//we should update the new data from isp for options of ours, not cascade
			if(server_config.upateConfig_isp==1){
				if(!strcmp(option->name, "domain")){
					if (existing->data[OPT_LEN] + length <= 255) {
						//printf("the orig length=%d\n", existing->data[OPT_LEN] );
					existing->data = realloc(existing->data, length + 2);
					memcpy(existing->data+2, buffer, length);
					existing->data[OPT_LEN] = length;
				} 
				}
			}
		}
#endif		
	} else {
		DEBUG(LOG_INFO, "Attaching option %s to list", option->name);
		
		/* make a new option */
		new = xmalloc(sizeof(struct option_set));
		new->data = xmalloc(length + 2);
		new->data[OPT_CODE] = option->code;
		new->data[OPT_LEN] = length;
		memcpy(new->data + 2, buffer, length);
		
		curr = opt_list;
		while (*curr && (*curr)->data[OPT_CODE] < option->code)
			curr = &(*curr)->next;
			
		new->next = *curr;
		*curr = new;		
#if defined(CONFIG_RTL865X_KLD)		
		if(!strcmp(option->name, "dns")){
			if(server_config.upateConfig_isp_dns == 1){
						if(update_lease_time1 ==1){
							update_option_dns++;
						}
					}
				}
#endif
	}
}

#ifdef AEI_TR111
int CreateOption125(int type, char *oui, char *sn, char *prod, char *VIinfo)
{
    char optionData[VENDOR_IDENTIFYING_INFO_LEN], *dataPtr;
    int len, totalLen = 0;
    //  char line[VENDOR_IDENTIFYING_INFO_LEN];

    optionData[VENDOR_OPTION_CODE_OFFSET] = (char)VENDOR_IDENTIFYING_OPTION_CODE;
    *(unsigned int*)(optionData+VENDOR_OPTION_ENTERPRISE_OFFSET) = (unsigned int)VENDOR_ENTERPRISE_NUMBER;
    dataPtr = optionData + VENDOR_OPTION_DATA_OFFSET + VENDOR_OPTION_DATA_LEN;
    totalLen = VENDOR_ENTERPRISE_LEN + VENDOR_OPTION_DATA_LEN;
    /* read system information and add it to option data */
    /* OUI */
    if (type == VENDOR_IDENTIFYING_FOR_DEVICE)
        *dataPtr++ = (char)VENDOR_DEVICE_OUI_SUBCODE;
    else
        *dataPtr++ = (char)VENDOR_GATEWAY_OUI_SUBCODE;
    len = strlen(oui);
    *dataPtr++ = len;
    strncpy(dataPtr,oui,len);
    dataPtr += len;
    totalLen += (len + VENDOR_SUBCODE_AND_LEN_BYTES);

    /* Serial Number */
    if (type == VENDOR_IDENTIFYING_FOR_DEVICE)
        *dataPtr++ = (char)VENDOR_DEVICE_SERIAL_NUMBER_SUBCODE;
    else
        *dataPtr++ = (char)VENDOR_GATEWAY_SERIAL_NUMBER_SUBCODE;
    len = strlen(sn);
    *dataPtr++ = len;
    strncpy(dataPtr,(const char*)sn,len);
    dataPtr += len;
    totalLen += (len + VENDOR_SUBCODE_AND_LEN_BYTES);

    /* Product Class */
    if (type == VENDOR_IDENTIFYING_FOR_DEVICE)
        *dataPtr++ = VENDOR_DEVICE_PRODUCT_CLASS_SUBCODE;
    else
        *dataPtr++ = VENDOR_GATEWAY_PRODUCT_CLASS_SUBCODE;
    len = strlen(prod);
    *dataPtr++ = len;
    strncpy(dataPtr,(const char*)prod,len);
    dataPtr += len;
    totalLen += (len + VENDOR_SUBCODE_AND_LEN_BYTES);

    optionData[VENDOR_OPTION_LEN_OFFSET] = totalLen;
    optionData[VENDOR_OPTION_DATA_OFFSET] = totalLen - VENDOR_ENTERPRISE_LEN - VENDOR_OPTION_DATA_LEN;

    /* also copy the option code and option len which is not counted in total len */
    memcpy((void*)VIinfo,(void*)optionData,(totalLen+VENDOR_OPTION_DATA_OFFSET));

    return 0;
}

int GetSvrVIoption(char *option)
{
    char *optionPtr;
    int maxSubcode = 3;
    int parsedLen = 0;
    int subcodeParsed = 0;
    int subcode, sublen;
    int optionLen;
    int ret = 0;
    char svroui_125[10]="";
    char svrsn_125[64]="";
    char svrprod_125[64]="";

    if (option == NULL) {
        DEBUG(LOG_ERR, "GetSvrVIoption(): option is NULL.");
        return -1;
    }

    optionPtr = option;
    optionPtr += VENDOR_ENTERPRISE_LEN;
    optionLen = *optionPtr;
    optionPtr +=  VENDOR_OPTION_DATA_LEN;

    while ((subcodeParsed < maxSubcode) && (parsedLen <= optionLen)) {
        /* subcode, len, data */
        subcode = *optionPtr++;
        sublen = *optionPtr++;
        subcodeParsed++;
        parsedLen += (sublen + VENDOR_OPTION_SUBCODE_LEN);

        switch (subcode)   {
            case VENDOR_DEVICE_OUI_SUBCODE:
            case VENDOR_GATEWAY_OUI_SUBCODE:
                if (sublen <= VENDOR_GATEWAY_OUI_MAX_LEN)  {
                    memcpy(svroui_125,optionPtr,sublen);
                    svroui_125[sublen] = '\0';
                    DEBUG(LOG_INFO, "svroui_125=%s", svroui_125);
                }  else {
                    DEBUG(LOG_ERR, "GetSvrVIoption(): subcode OUI, OUI len %d is too long.",sublen);
                    goto viError;
                }
                break;
            case VENDOR_DEVICE_SERIAL_NUMBER_SUBCODE:
            case VENDOR_GATEWAY_SERIAL_NUMBER_SUBCODE:
                if (sublen <= VENDOR_GATEWAY_SERIAL_NUMBER_MAX_LEN)   {
                    memcpy(svrsn_125,optionPtr,sublen);
                    svrsn_125[sublen] = '\0';
                    DEBUG(LOG_INFO, "svrsn_125=%s", svrsn_125);
                }   else  {
                    DEBUG(LOG_ERR, "GetSvrVIoption(): subcode SerialNumber, Serial Number len %d is too long.",sublen);
                    goto viError;
                }
                break;
            case VENDOR_DEVICE_PRODUCT_CLASS_SUBCODE:
            case VENDOR_GATEWAY_PRODUCT_CLASS_SUBCODE:
                if (sublen <= VENDOR_GATEWAY_PRODUCT_CLASS_MAX_LEN) {
                    memcpy(svrprod_125,optionPtr,sublen);
                    svrprod_125[sublen] = '\0';
                    DEBUG(LOG_INFO, "svrprod_125=%s", svrprod_125);
                }   else  {
                    DEBUG(LOG_ERR, "GetSvrVIoption(): subcode ProductClass, Class len %d is too long.",sublen);
                    goto viError;
                }
                break;
            default:
                DEBUG(LOG_ERR, "GetSvrVIoption(): subcode %d, not supported.",subcode);
                goto viError;
        }
        optionPtr += sublen;
        /* add info to the manageable device link list */
    } /* while subcodeParsed < maxSubcode */


#ifdef AEI_TR111                         
// for TR111 requirement
    tr69_commit_set_leaf_data( "Device.GatewayInfo.ManufacturerOUI", (void **)svroui_125, TR69_NODE_LEAF_TYPE_STRING);
    tr69_commit_set_leaf_data( "Device.GatewayInfo.SerialNumber", (void **)svrsn_125, TR69_NODE_LEAF_TYPE_STRING);
    tr69_commit_set_leaf_data( "Device.GatewayInfo.ProductClass", (void **)svrprod_125, TR69_NODE_LEAF_TYPE_STRING);
    tr69_commit_set_leaf_data_end( "Device.GatewayInfo");
#endif

    return ret;

viError:
    ret = -1;
    return ret;
}

#endif


