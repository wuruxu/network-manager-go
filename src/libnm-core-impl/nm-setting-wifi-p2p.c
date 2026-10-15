/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * Copyright (C) 2019 Red Hat, Inc.
 */

#include "libnm-core-impl/nm-default-libnm-core.h"

#include "nm-setting-wifi-p2p.h"

#include <net/ethernet.h>

#include "nm-utils.h"
#include "libnm-core-aux-intern/nm-common-macros.h"
#include "nm-utils-private.h"
#include "nm-setting-private.h"

/**
 * SECTION:nm-setting-wifi-p2p
 * @short_description: Describes connection properties for 802.11 Wi-Fi P2P networks
 *
 * The #NMSettingWifiP2P object is a #NMSetting subclass that describes properties
 * necessary for connection to 802.11 Wi-Fi P2P networks (aka Wi-Fi Direct).
 **/

/**
 * NMSettingWifiP2P:
 *
 * Wi-Fi P2P Settings
 *
 * Since: 1.16
 */

/*****************************************************************************/

NM_GOBJECT_PROPERTIES_DEFINE_BASE(PROP_PEER, PROP_WPS_METHOD, PROP_WFD_IES, PROP_AUTONOMOUS, PROP_FREQUENCY, PROP_CLIENT, PROP_SSID, PROP_PSK, PROP_PSK_FLAGS, );

typedef struct {
    char   *peer;
    GBytes *wfd_ies;
    guint32 wps_method;
    bool autonomous;
    guint32 frequency;
    bool client;
    GBytes *ssid;
    char *psk;
    NMSettingSecretFlags psk_flags;
} NMSettingWifiP2PPrivate;

struct _NMSettingWifiP2P {
    NMSetting               parent;
    NMSettingWifiP2PPrivate _priv;
};

struct _NMSettingWifiP2PClass {
    NMSettingClass parent;
};

G_DEFINE_TYPE(NMSettingWifiP2P, nm_setting_wifi_p2p, NM_TYPE_SETTING)

#define NM_SETTING_WIFI_P2P_GET_PRIVATE(self) \
    _NM_GET_PRIVATE(self, NMSettingWifiP2P, NM_IS_SETTING_WIFI_P2P, NMSetting)

/*****************************************************************************/

/**
 * nm_setting_wifi_p2p_get_peer:
 * @setting: the #NMSettingWifiP2P
 *
 * Returns: the #NMSettingWifiP2P:peer property of the setting
 *
 * Since: 1.16
 **/
const char *
nm_setting_wifi_p2p_get_peer(NMSettingWifiP2P *setting)
{
    g_return_val_if_fail(NM_IS_SETTING_WIFI_P2P(setting), NULL);

    return NM_SETTING_WIFI_P2P_GET_PRIVATE(setting)->peer;
}

/**
 * nm_setting_wifi_p2p_get_wps_method:
 * @setting: the #NMSettingWifiP2P
 *
 * Returns: the #NMSettingWifiP2P:wps-method property of the setting
 *
 * Since: 1.16
 **/
NMSettingWirelessSecurityWpsMethod
nm_setting_wifi_p2p_get_wps_method(NMSettingWifiP2P *setting)
{
    g_return_val_if_fail(NM_IS_SETTING_WIFI_P2P(setting),
                         NM_SETTING_WIRELESS_SECURITY_WPS_METHOD_DEFAULT);

    return NM_SETTING_WIFI_P2P_GET_PRIVATE(setting)->wps_method;
}

/**
 * nm_setting_wifi_p2p_get_wfd_ies:
 * @setting: the #NMSettingWiFiP2P
 *
 * Returns: (transfer none): the #NMSettingWiFiP2P:wfd-ies property of the setting
 *
 * Since: 1.16
 **/
GBytes *
nm_setting_wifi_p2p_get_wfd_ies(NMSettingWifiP2P *setting)
{
    g_return_val_if_fail(NM_IS_SETTING_WIFI_P2P(setting), NULL);

    return NM_SETTING_WIFI_P2P_GET_PRIVATE(setting)->wfd_ies;
}

/**
 * nm_setting_wifi_p2p_get_autonomous:
 * @setting: the #NMSettingWifiP2P
 *
 * Returns: whether to create an autonomous group owner without a peer
 *
 * Downstream extension to NetworkManager 1.58.1.
 **/
gboolean
nm_setting_wifi_p2p_get_autonomous(NMSettingWifiP2P *setting)
{
    g_return_val_if_fail(NM_IS_SETTING_WIFI_P2P(setting), FALSE);
    return NM_SETTING_WIFI_P2P_GET_PRIVATE(setting)->autonomous;
}

/**
 * nm_setting_wifi_p2p_get_frequency:
 * @setting: the #NMSettingWifiP2P
 *
 * Returns: the requested GO frequency in MHz, or zero for automatic selection
 *
 * Downstream extension to NetworkManager 1.58.1.
 **/
guint32
nm_setting_wifi_p2p_get_frequency(NMSettingWifiP2P *setting)
{
    g_return_val_if_fail(NM_IS_SETTING_WIFI_P2P(setting), 0);
    return NM_SETTING_WIFI_P2P_GET_PRIVATE(setting)->frequency;
}

/*****************************************************************************/

gboolean
nm_setting_wifi_p2p_get_client(NMSettingWifiP2P *setting)
{
    g_return_val_if_fail(NM_IS_SETTING_WIFI_P2P(setting), FALSE);
    return NM_SETTING_WIFI_P2P_GET_PRIVATE(setting)->client;
}

GBytes *
nm_setting_wifi_p2p_get_ssid(NMSettingWifiP2P *setting)
{
    g_return_val_if_fail(NM_IS_SETTING_WIFI_P2P(setting), NULL);
    return NM_SETTING_WIFI_P2P_GET_PRIVATE(setting)->ssid;
}

const char *
nm_setting_wifi_p2p_get_psk(NMSettingWifiP2P *setting)
{
    g_return_val_if_fail(NM_IS_SETTING_WIFI_P2P(setting), NULL);
    return NM_SETTING_WIFI_P2P_GET_PRIVATE(setting)->psk;
}

NMSettingSecretFlags
nm_setting_wifi_p2p_get_psk_flags(NMSettingWifiP2P *setting)
{
    g_return_val_if_fail(NM_IS_SETTING_WIFI_P2P(setting), NM_SETTING_SECRET_FLAG_NONE);
    return NM_SETTING_WIFI_P2P_GET_PRIVATE(setting)->psk_flags;
}

static gboolean
verify(NMSetting *setting, NMConnection *connection, GError **error)
{
    NMSettingWifiP2PPrivate *priv = NM_SETTING_WIFI_P2P_GET_PRIVATE(setting);

    if (!priv->autonomous && !priv->client && !priv->peer) {
        g_set_error_literal(error,
                            NM_CONNECTION_ERROR,
                            NM_CONNECTION_ERROR_MISSING_PROPERTY,
                            _("property is missing"));
        g_prefix_error(error,
                       "%s.%s: ",
                       NM_SETTING_WIFI_P2P_SETTING_NAME,
                       NM_SETTING_WIFI_P2P_PEER);
        return FALSE;
    }

    if (((priv->autonomous || priv->client) && priv->peer)
        || (priv->peer && !nm_utils_hwaddr_valid(priv->peer, ETH_ALEN))) {
        g_set_error_literal(error,
                            NM_CONNECTION_ERROR,
                            NM_CONNECTION_ERROR_INVALID_PROPERTY,
                            _("property is invalid"));
        g_prefix_error(error,
                       "%s.%s: ",
                       NM_SETTING_WIFI_P2P_SETTING_NAME,
                       NM_SETTING_WIFI_P2P_PEER);
        return FALSE;
    }

    if ((!priv->autonomous && !priv->client && priv->frequency)
        || (priv->frequency && !nm_utils_wifi_freq_to_channel(priv->frequency))) {
        g_set_error_literal(error, NM_CONNECTION_ERROR,
                            NM_CONNECTION_ERROR_INVALID_PROPERTY, _("property is invalid"));
        g_prefix_error(error, "%s.%s: ", NM_SETTING_WIFI_P2P_SETTING_NAME,
                       NM_SETTING_WIFI_P2P_FREQUENCY);
        return FALSE;
    }

    if ((priv->client && (priv->psk_flags & NM_SETTING_SECRET_FLAG_NOT_REQUIRED))
        || (priv->client && (priv->autonomous || !priv->ssid
                         || g_bytes_get_size(priv->ssid) < 1 || g_bytes_get_size(priv->ssid) > 32))
        || (!priv->client && (priv->ssid || priv->psk || priv->psk_flags))
        || (priv->client && priv->wps_method != NM_SETTING_WIRELESS_SECURITY_WPS_METHOD_DEFAULT
            && priv->wps_method != NM_SETTING_WIRELESS_SECURITY_WPS_METHOD_DISABLED)) {
        g_set_error_literal(error, NM_CONNECTION_ERROR, NM_CONNECTION_ERROR_INVALID_PROPERTY,
                            "wifi-p2p: credential client requires a 1-32 byte SSID and excludes GO, peer and WPS");
        return FALSE;
    }

    if (!_nm_utils_wps_method_validate(priv->wps_method,
                                       NM_SETTING_WIFI_P2P_SETTING_NAME,
                                       NM_SETTING_WIFI_P2P_WPS_METHOD,
                                       TRUE,
                                       error))
        return FALSE;

    return TRUE;
}

/*****************************************************************************/

static gboolean
verify_secrets(NMSetting *setting, NMConnection *connection, GError **error)
{
    NMSettingWifiP2PPrivate *priv = NM_SETTING_WIFI_P2P_GET_PRIVATE(setting);
    gsize len;
    if (!priv->psk)
        return TRUE;
    len = strlen(priv->psk);
    if (len >= 8 && len <= 63)
        return TRUE;
    if (len == 64 && strspn(priv->psk, "0123456789abcdefABCDEF") == 64)
        return TRUE;
    g_set_error_literal(error, NM_CONNECTION_ERROR, NM_CONNECTION_ERROR_INVALID_PROPERTY,
                        "wifi-p2p.psk: expected an 8-63 byte passphrase or 64 hexadecimal digits");
    return FALSE;
}

static GPtrArray *
need_secrets(NMSetting *setting, gboolean check_rerequest)
{
    NMSettingWifiP2PPrivate *priv = NM_SETTING_WIFI_P2P_GET_PRIVATE(setting);
    GPtrArray *secrets;
    if (!priv->client || (!check_rerequest && priv->psk))
        return NULL;
    secrets = g_ptr_array_sized_new(1);
    g_ptr_array_add(secrets, NM_SETTING_WIFI_P2P_PSK);
    return secrets;
}

static void
nm_setting_wifi_p2p_init(NMSettingWifiP2P *setting)
{}

/**
 * nm_setting_wifi_p2p_new:
 *
 * Creates a new #NMSettingWifiP2P object with default values.
 *
 * Returns: (transfer full): the new empty #NMSettingWifiP2P object
 *
 * Since: 1.16
 **/
NMSetting *
nm_setting_wifi_p2p_new(void)
{
    return g_object_new(NM_TYPE_SETTING_WIFI_P2P, NULL);
}

static void
nm_setting_wifi_p2p_class_init(NMSettingWifiP2PClass *setting_wifi_p2p_class)
{
    GObjectClass   *object_class        = G_OBJECT_CLASS(setting_wifi_p2p_class);
    NMSettingClass *setting_class       = NM_SETTING_CLASS(setting_wifi_p2p_class);
    GArray         *properties_override = _nm_sett_info_property_override_create_array();

    object_class->get_property = _nm_setting_property_get_property_direct;
    object_class->set_property = _nm_setting_property_set_property_direct;

    setting_class->verify = verify;
    setting_class->verify_secrets = verify_secrets;
    setting_class->need_secrets = need_secrets;

    /**
     * NMSettingWifiP2P:peer:
     *
     * The P2P device that should be connected to. This is required for peer
     * connections and must be unset when autonomous is TRUE.
     *
     * Since: 1.16
     */
    /* ---keyfile---
     * property: peer
     * format: usual hex-digits-and-colons notation
     * description: MAC address in traditional hex-digits-and-colons notation
     *   (e.g. 00:22:68:12:79:A2), or semicolon separated list of 6 bytes (obsolete)
     *   (e.g. 0;34;104;18;121;162).
     * ---end---
     */
    _nm_setting_property_define_direct_string(properties_override,
                                              obj_properties,
                                              NM_SETTING_WIFI_P2P_PEER,
                                              PROP_PEER,
                                              NM_SETTING_PARAM_NONE,
                                              NMSettingWifiP2P,
                                              _priv.peer,
                                              .direct_string_allow_empty = TRUE);

    /**
     * NMSettingWifiP2P:wps-method:
     *
     * Flags indicating which mode of WPS is to be used.
     *
     * There's little point in changing the default setting as NetworkManager will
     * automatically determine the best method to use.
     *
     * Since: 1.16
     */
    _nm_setting_property_define_direct_uint32(properties_override,
                                              obj_properties,
                                              NM_SETTING_WIFI_P2P_WPS_METHOD,
                                              PROP_WPS_METHOD,
                                              0,
                                              G_MAXUINT32,
                                              NM_SETTING_WIRELESS_SECURITY_WPS_METHOD_DEFAULT,
                                              NM_SETTING_PARAM_FUZZY_IGNORE,
                                              NMSettingWifiP2P,
                                              _priv.wps_method);

    /**
     * NMSettingWifiP2P:wfd-ies:
     *
     * The Wi-Fi Display (WFD) Information Elements (IEs) to set.
     *
     * Wi-Fi Display requires a protocol specific information element to be
     * set in certain Wi-Fi frames. These can be specified here for the
     * purpose of establishing a connection.
     * This setting is only useful when implementing a Wi-Fi Display client.
     *
     * Since: 1.16
     */
    _nm_setting_property_define_direct_bytes(properties_override,
                                             obj_properties,
                                             NM_SETTING_WIFI_P2P_WFD_IES,
                                             PROP_WFD_IES,
                                             NM_SETTING_PARAM_FUZZY_IGNORE,
                                             NMSettingWifiP2P,
                                             _priv.wfd_ies);

    /**
     * NMSettingWifiP2P:autonomous:
     *
     * Create a non-persistent autonomous group owner without a peer. The group
     * remains active when no clients are present, until the connection is
     * deactivated. Uses the wpa_supplicant backend. Downstream 1.58.1 extension.
     */
    _nm_setting_property_define_direct_boolean(properties_override,
                                               obj_properties,
                                               NM_SETTING_WIFI_P2P_AUTONOMOUS,
                                               PROP_AUTONOMOUS,
                                               FALSE,
                                               NM_SETTING_PARAM_NONE,
                                               NMSettingWifiP2P,
                                               _priv.autonomous);

    /**
     * NMSettingWifiP2P:frequency:
     *
     * Requested autonomous GO frequency in MHz. Zero follows the current
     * station frequency when connected, otherwise lets supplicant choose.
     * An explicit frequency conflicting with an active station is rejected.
     * Also accepts an existing group frequency in credential client mode; zero scans for it. Downstream 1.58.1 extension.
     */
    _nm_setting_property_define_direct_uint32(properties_override,
                                              obj_properties,
                                              NM_SETTING_WIFI_P2P_FREQUENCY,
                                              PROP_FREQUENCY,
                                              0, 7125, 0,
                                              NM_SETTING_PARAM_NONE,
                                              NMSettingWifiP2P,
                                              _priv.frequency);

    /**
     * NMSettingWifiP2P:client:
     *
     * Join an existing group with SSID and PSK using a separate P2P client
     * interface. Excludes autonomous GO, peer and WPS. Downstream extension.
     */
    _nm_setting_property_define_direct_boolean(properties_override, obj_properties,
        NM_SETTING_WIFI_P2P_CLIENT, PROP_CLIENT, FALSE, NM_SETTING_PARAM_NONE,
        NMSettingWifiP2P, _priv.client);
    /**
     * NMSettingWifiP2P:ssid:
     *
     * Raw SSID of the existing group, required in credential client mode.
     */
    _nm_setting_property_define_direct_bytes(properties_override, obj_properties,
        NM_SETTING_WIFI_P2P_SSID, PROP_SSID, NM_SETTING_PARAM_NONE,
        NMSettingWifiP2P, _priv.ssid);
    /**
     * NMSettingWifiP2P:psk:
     *
     * WPA passphrase or hexadecimal PSK for the existing group. Client only.
     */
    _nm_setting_property_define_direct_string(properties_override, obj_properties,
        NM_SETTING_WIFI_P2P_PSK, PROP_PSK, NM_SETTING_PARAM_SECRET,
        NMSettingWifiP2P, _priv.psk);
    /**
     * NMSettingWifiP2P:psk-flags:
     *
     * Secret handling flags for the group PSK.
     */
    _nm_setting_property_define_direct_secret_flags(properties_override, obj_properties,
        NM_SETTING_WIFI_P2P_PSK_FLAGS, PROP_PSK_FLAGS, NMSettingWifiP2P, _priv.psk_flags);

    g_object_class_install_properties(object_class, _PROPERTY_ENUMS_LAST, obj_properties);

    _nm_setting_class_commit(setting_class,
                             NM_META_SETTING_TYPE_WIFI_P2P,
                             NULL,
                             properties_override,
                             0);
}
