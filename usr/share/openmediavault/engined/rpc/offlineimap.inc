<?php
/**
 * @license   http://www.gnu.org/licenses/gpl.html GPL Version 3
 * @author    Volker Theile <volker.theile@openmediavault.org>
 * @author    OpenMediaVault Plugin Developers <plugins@omv-extras.org>
 * @copyright Copyright (c) 2009-2013 Volker Theile
 * @copyright Copyright (c) 2013-2014 OpenMediaVault Plugin Developers
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
require_once("openmediavault/object.inc");
require_once("openmediavault/config.inc");
require_once("openmediavault/error.inc");
require_once("openmediavault/util.inc");
require_once("openmediavault/rpcservice.inc");
require_once("openmediavault/notify.inc");

class OMVRpcServiceOfflineImap extends OMVRpcServiceAbstract {

    public function getName() {
        return "OfflineImap";
    }

    public function initialize() {
        $this->registerMethod("getAccounts");
        $this->registerMethod("getAccount");
        $this->registerMethod("setAccount");
        $this->registerMethod("deleteAccount");

        $this->registerMethod("doSync");
        $this->registerMethod("doSyncSilent");
    }

    public function getAccounts($params, $context) {
        global $xmlConfig;

        // Validate the RPC caller context.
        $this->validateMethodContext($context, array( "role" => OMV_ROLE_ADMINISTRATOR ));

        // Validate the parameters of the RPC service method.
        $this->validateMethodParams($params, '{
            "type"       : "object",
            "properties" : {
                "start"     : { "type" : "integer" },
                "limit"     : { '.$GLOBALS['OMV_JSONSCHEMA_COUNTFIELD'].' },
                "sortfield" : { '.$GLOBALS['OMV_JSONSCHEMA_SORTFIELD'].' },
                "sortdir"   : { '.$GLOBALS['OMV_JSONSCHEMA_SORTDIR'].' }
            }
        }');

        // Get configuration data.
        $xpath = "//services/offlineimap/accounts/account";
        $objects = $xmlConfig->getList($xpath);
        if(is_null($objects))
            throw new OMVException(OMVErrorMsg::E_CONFIG_GET_OBJECT_FAILED, $xpath);

        // Add additional share informations.
        foreach($objects as $objectk => &$objectv) {
            $objectv['sharedfoldername'] = gettext("n/a");
            // Get the shared folder configuration object.
            $sfObject = $xmlConfig->get("//system/shares/sharedfolder[uuid='{$objectv['sharedfolderref']}']");

            if(!is_null($sfObject) && !empty($sfObject))
                $objectv['sharedfoldername'] = $sfObject['name'];
        }

        // Filter the result.
        return $this->applyFilter($objects, $params['start'], $params['limit'],
            $params['sortfield'], $params['sortdir']);
    }

    public function getAccount($params, $context) {
        global $xmlConfig;

        // Validate the RPC caller context.
        $this->validateMethodContext($context, array("role" => OMV_ROLE_ADMINISTRATOR));

        // Validate the parameters of the RPC service method.
        $this->validateMethodParams($params, '{
            "type"       : "object",
            "properties" : {
                "uuid" : { '.$GLOBALS['OMV_JSONSCHEMA_UUID'].' }
            }
        }');

        // Get the configuration object.
        $xpath = sprintf("//services/offlineimap/accounts/account[uuid='%s']", $params['uuid']);
        $object = $xmlConfig->get($xpath);
        if(is_null($object))
            throw new OMVException(OMVErrorMsg::E_CONFIG_GET_OBJECT_FAILED, $xpath);

        // Modify result data
        $object['ssl']  = boolval($object['ssl']);
        $object['port'] = intval($object['port']);

        return $object;
    }

    public function setAccount($params, $context) {
        global $xmlConfig;

        // Validate the RPC caller context.
        $this->validateMethodContext($context, array( "role" => OMV_ROLE_ADMINISTRATOR ));

        // Validate the parameters of the RPC service method.
        $this->validateMethodParams($params, '{
            "type"       : "object",
            "properties" : {
                "uuid"            : { '.$GLOBALS['OMV_JSONSCHEMA_UUID_UNDEFINED'].' },
                "name"            : { "type" : "string" },
                "host"            : { "type" : "string" },
                "user"            : { "type" : "string" },
                "pass"            : { "type" : "string" },
                "ssl"             : { "type" : "boolean" },
                "port"            : { "type" : "integer" },
                "sharedfolderref" : { '.$GLOBALS['OMV_JSONSCHEMA_UUID'].' }
            }
        }');

        // Prepare configuration data.
        $object = array(
            "uuid"            => ($params['uuid'] == $GLOBALS['OMV_UUID_UNDEFINED']) ? OMVUtil::uuid() : $params['uuid'],
            "name"            => $params['name'],
            "host"            => $params['host'],
            "user"            => $params['user'],
            "pass"            => $params['pass'],
            "ssl"             => array_boolval($params, 'ssl'),
            "port"            => intval($params['port']),
            "sharedfolderref" => $params['sharedfolderref']
        );

        // Set the configuration data.
        $success = FALSE;
        if($params['uuid'] == $GLOBALS['OMV_UUID_UNDEFINED']) {
            // Append object to configuration.
            $success = $xmlConfig->set("//services/offlineimap/accounts", array("account" => $object));
        } else {
            // Update existing configuration object.
            $success = $xmlConfig->replace("//services/offlineimap/accounts/account[uuid='{$params['uuid']}']", $object);
        }

        if(FALSE === $success)
            throw new OMVException(OMVErrorMsg::E_CONFIG_SET_OBJECT_FAILED);

        // Notify configuration changes.
        $dispatcher = &OMVNotifyDispatcher::getInstance();
        $dispatcher->notify(($params['uuid'] == $GLOBALS['OMV_UUID_UNDEFINED']) ?
            OMV_NOTIFY_CREATE : OMV_NOTIFY_MODIFY,
            "org.openmediavault.services.offlineimap.accounts.account", $object);

        // Return the configuration object.
        return $object;
    }


    public function deleteAccount($params, $context) {
        global $xmlConfig;

        // Validate the RPC caller context.
        $this->validateMethodContext($context, array( "role" => OMV_ROLE_ADMINISTRATOR ));

        // Validate the parameters of the RPC service method.
        $this->validateMethodParams($params, '{
            "type"       : "object",
            "properties" : {
                "uuid" : { '.$GLOBALS['OMV_JSONSCHEMA_UUID'].' }
            }
        }');

        // Delete the configuration object.
        $xpath = sprintf("//services/offlineimap/accounts/account[uuid='%s']", $params['uuid']);
        return $this->deleteConfigObjectByPath($xpath,
            "org.openmediavault.services.offlineimap.accounts.account");
    }


    function doSyncSilent($params, $context) {
        // Validate the RPC caller context.
        $this->validateMethodContext($context, array( "role" => OMV_ROLE_ADMINISTRATOR ));

        // Validate the parameters of the RPC service method.
        $this->validateMethodParams($params, '{
            "type"       : "object",
            "properties" : {
                "name" : { "type" : "string" }
            }
        }');

        $name = $params['name'];

        $cmd = "/usr/bin/offlineimap -o -c /root/.offlineimaprc -a $name";
        if(0 !== $this->exec($cmd, $output))
            throw new OMVException(OMVErrorMsg::E_EXEC_FAILED, $cmd, implode("\n", $output));
    }

    function doSync($params, $context) {
        // Validate the RPC caller context.
        $this->validateMethodContext($context, array( "role" => OMV_ROLE_ADMINISTRATOR ));

        // Validate the parameters of the RPC service method.
        $this->validateMethodParams($params, '{
            "type"       : "object",
            "properties" : {
                "name" : { "type" : "string" }
            }
        }');

        $name = $params['name'];

        // Create a background process.
        $bgStatusFilename = $this->createBgProcStatus();
        $pid = $this->fork();
        if($pid > 0) { // Parent process.
            $this->initializeBgProcStatus($bgStatusFilename, $pid);
            return $bgStatusFilename;
        }

        // Child process.
        try {
            $bgOutputFilename = $this->createBgProcOutput();
            $this->updateBgProcStatus($bgStatusFilename, "outputfilename", $bgOutputFilename);

            $cmd = "/usr/bin/offlineimap -o -c /root/.offlineimaprc -a $name 2>&1";
            $this->exec($cmd, $output, $bgOutputFilename);
            $this->finalizeBgProcStatus($bgStatusFilename, $output);

        } catch(Exception $e) {
            $this->finalizeBgProcStatus($bgStatusFilename, "", $e);
            exit(1);
        }
    }
}

// Register the RPC service.
$rpcServiceMgr = &OMVRpcServiceMgr::getInstance();
$rpcServiceMgr->registerService(new OMVRpcServiceOfflineImap());
?>
