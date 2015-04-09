/**
 * @license   http://www.gnu.org/licenses/gpl.html GPL Version 3
 * @author    Volker Theile <volker.theile@openmediavault.org>
 * @author    OpenMediaVault Plugin Developers <plugins@omv-extras.org>
 * @copyright Copyright (c) 2009-2013 Volker Theile
 * @copyright Copyright (c) 2014-2015 OpenMediaVault Plugin Developers
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
// require("js/omv/WorkspaceManager.js")
// require("js/omv/workspace/grid/Panel.js")
// require("js/omv/workspace/window/Form.js")
// require("js/omv/workspace/window/plugin/ConfigObject.js")
// require("js/omv/util/Format.js")
// require("js/omv/Rpc.js")
// require("js/omv/data/Store.js")
// require("js/omv/data/Model.js")
// require("js/omv/data/proxy/Rpc.js")
// require("js/omv/window/Execute.js")
// require("js/omv/form/field/SharedFolderComboBox.js")

Ext.define("OMV.module.admin.service.offlineimap.Account", {
    extend   : "OMV.workspace.window.Form",
    requires : [
        "OMV.workspace.window.plugin.ConfigObject"
    ],

    rpcService   : "OfflineImap",
    rpcGetMethod : "getAccount",
    rpcSetMethod : "setAccount",
    plugins      : [{
        ptype : "configobject"
    }],

    getFormItems : function() {
        return [{
            xtype      : "textfield",
            name       : "name",
            fieldLabel : _("Name"),
            allowBlank : false
        },{
            xtype      : "textfield",
            name       : "host",
            fieldLabel : _("Hostname"),
            allowBlank : false
        },{
            xtype      : "textfield",
            name       : "user",
            fieldLabel : _("Username"),
            allowBlank : false
        },{
            xtype      : "passwordfield",
            name       : "pass",
            fieldLabel : _("Password"),
            allowBlank : false
        },{
            xtype      : "checkbox",
            name       : "ssl",
            fieldLabel : _("SSL"),
            checked    : false
        },{
            xtype      : "numberfield",
            name       : "port",
            fieldLabel : _("IMAP Port"),
            value      : 25
        },{
            xtype      : "sharedfoldercombo",
            name       : "sharedfolderref",
            fieldLabel : _("Shared Folder"),
            allowBlank : false,
            allowNone  : false
        }];
    }
});

Ext.define("OMV.module.admin.service.offlineimap.Accounts", {
    extend   : "OMV.workspace.grid.Panel",
    requires : [
        "OMV.Rpc",
        "OMV.data.Store",
        "OMV.data.Model",
        "OMV.data.proxy.Rpc",
        "OMV.util.Format"
    ],
    uses     : [
        "OMV.module.admin.service.offlineimap.Account"
    ],

    hidePagingToolbar : false,
    stateful          : true,
    stateId           : "a123a76d-6804-4152-b31b-8b74c0ea6dde",
    columns           : [{
        text      : _("Name"),
        sortable  : true,
        dataIndex : "name",
        stateId   : "name"
    },{
        text      : _("Host"),
        sortable  : true,
        dataIndex : "host",
        stateId   : "host"
    },{
        text      : _("User"),
        sortable  : true,
        dataIndex : "user",
        stateId   : "user"
    },{
        xtype     : "booleaniconcolumn",
        header    : _("SSL"),
        sortable  : true,
        dataIndex : "ssl",
        align     : "center",
        width     : 80,
        resizable : false,
        trueIcon  : "switch_on.png",
        falseIcon : "switch_off.png"
    },{
        text      : _("Port"),
        sortable  : true,
        dataIndex : "port",
        align     : "center",
        width     : 80,
        resizable : false,
        stateId   : "port"
    },{
        text      : _("Shared Folder"),
        sortable  : true,
        dataIndex : "sharedfoldername",
        stateId   : "sharedfoldername"
    }],

    initComponent : function() {
        var me = this;
        Ext.apply(me, {
            store : Ext.create("OMV.data.Store", {
                autoLoad : true,
                model    : OMV.data.Model.createImplicit({
                    idProperty  : "uuid",
                    fields      : [
                        { name : "uuid", type : "string" },
                        { name : "name", type : "string" },
                        { name : "host", type : "string" },
                        { name : "user", type : "string" },
                        { name : "ssl", type : "boolean" },
                        { name : "port", type : "string" },
                        { name : "sharedfoldername", type: "string" }
                    ]
                }),
                proxy : {
                    type    : "rpc",
                    rpcData : {
                        service : "OfflineImap",
                        method  : "getAccounts"
                    }
                }
            })
        });
        me.callParent(arguments);
    },

    getTopToolbarItems: function() {
        var me = this;
        var items = me.callParent(arguments);
        Ext.Array.push(items, {
            id: me.getId() + "-sync",
            xtype: "button",
            text: _("Sync"),
            icon: "images/refresh.png",
            iconCls: Ext.baseCSSPrefix + "btn-icon-16x16",
            handler: Ext.Function.bind(me.onSyncButton, me, [ me ]),
            scope: me
        },{
            id: me.getId() + "-silent",
            xtype: "button",
            text: _("Silent"),
            icon: "images/refresh.png",
            iconCls: Ext.baseCSSPrefix + "btn-icon-16x16",
            handler: Ext.Function.bind(me.onSilentButton, me, [ me ]),
            scope: me
        });
        return items;
    },

    onAddButton : function() {
        var me = this;
        Ext.create("OMV.module.admin.service.offlineimap.Account", {
            title     : _("Add account"),
            uuid      : OMV.UUID_UNDEFINED,
            listeners : {
                scope  : me,
                submit : function() {
                    this.doReload();
                }
            }
        }).show();
    },

    onEditButton : function() {
        var me = this;
        var record = me.getSelected();
        Ext.create("OMV.module.admin.service.offlineimap.Account", {
            title     : _("Edit account"),
            uuid      : record.get("uuid"),
            listeners : {
                scope  : me,
                submit : function() {
                    this.doReload();
                }
            }
        }).show();
    },

    doDeletion : function(record) {
        var me = this;
        OMV.Rpc.request({
            scope    : me,
            callback : me.onDeletion,
            rpcData  : {
                service : "OfflineImap",
                method  : "deleteAccount",
                params  : {
                    uuid : record.get("uuid")
                }
            }
        });
    },

    onSyncButton : function() {
        var me = this;
        var record = me.getSelected();
        var wnd = Ext.create("OMV.window.Execute", {
            title           : _("Syncing new mail ..."),
            rpcService      : "OfflineImap",
            rpcMethod       : "doSync",
            rpcParams       : {
                name : record.get("name")
            },
            rpcIgnoreErrors : true,
            hideStartButton : true,
            hideStopButton  : false,
            listeners       : {
                scope     : me,
                finish    : function(wnd, response) {
                    wnd.appendValue(_("Done..."));
                    wnd.setButtonDisabled("close", false);
                },
                exception : function(wnd, error) {
                    OMV.MessageBox.error(null, error);
                    wnd.setButtonDisabled("close", false);
                }
            }
        });
        wnd.setButtonDisabled("close", true);
        wnd.show();
        wnd.start();
    },

    onSilentButton : function() {
        var me = this;
        var record = me.getSelected();
        OMV.RpcObserver.request({
            msg     : _("Syncing new mail ..."),
            rpcData : {
                service : "OfflineImap",
                method  : "doSyncSilent",
                params  : {
                    name : record.get("name")
                }
            },
            scope   : me,
            finish  : function() {
                this.doReload();
            }
        });
    }
});

OMV.WorkspaceManager.registerPanel({
    id        : "accounts",
    path      : "/service/offlineimap",
    text      : _("Accounts"),
    position  : 10,
    className : "OMV.module.admin.service.offlineimap.Accounts"
});
