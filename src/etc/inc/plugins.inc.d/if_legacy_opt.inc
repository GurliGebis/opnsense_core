<?php

/*
    Copyright (C) 2016 Deciso B.V.
    All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are met:

    1. Redistributions of source code must retain the above copyright notice,
       this list of conditions and the following disclaimer.

    2. Redistributions in binary form must reproduce the above copyright
       notice, this list of conditions and the following disclaimer in the
       documentation and/or other materials provided with the distribution.

    THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
    INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
    AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
    AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
    OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
    SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
    INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
    CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
    ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
    POSSIBILITY OF SUCH DAMAGE.
*/

/**
 * options we should probably remove from the system at some point, lets make them plugabble before removal
 */
function if_legacy_opt_firewall($fw)
{
    global $config, $GatewaysList;

    $defaults = array();
    $defaults['pass'] = array("type" => "pass", "log" => !isset($config['syslog']['nologdefaultpass']));

    /*
     *  pass traffic between statically routed subnets and the subnet on the
     *  interface in question to avoid problems with complicated routing
     *  topologies
     */
    if (isset($config['filter']['bypassstaticroutes']) && isset($config['staticroutes']['route']) && count($config['staticroutes']['route'])) {
        $FilterIflist = filter_generate_optcfg_array();
        filter_generate_gateways(); // loads global $GatewaysList

        foreach (get_staticroutes() as $route) {
            $friendly = $GatewaysList[$route['gateway']]['friendlyiface'];
            if (is_array($FilterIflist[$friendly])) {
                $oc = $FilterIflist[$friendly];
                $routeent = explode("/", $route['network']);
                if (is_ipaddrv4($routeent[0]) && is_ipaddrv4($oc['ip'])) {
                    $sa = $oc['sa'];
                    $sn = $oc['sn'];
                } elseif (is_ipaddrv6($routeent[0]) && is_ipaddrv6($oc['ipv6'])) {
                    $sa = $oc['sav6'];
                    $sn = $oc['snv6'];
                } else {
                    continue;
                }

                $networks = array();
                $networks[] = array('from' => "{$sa}/{$sn}", 'to' => $route['network']);
                $networks[] = array('to' => "{$sa}/{$sn}", 'from' => $route['network']);
                foreach ($networks as $network) {
                    $fw->registerFilterRule(10,
                      array('interface' => $friendly, 'statetype' => 'sloppy',
                            'protocol' => 'tcp','flags' => 'any', 'from' => $network['from'],
                            'to' => $network['to'], 'quick' => false,
                            'label' => "pass traffic between statically routed subnets"),
                      $defaults['pass']
                    );
                    $fw->registerFilterRule(10,
                      array('interface' => $friendly, 'statetype' => 'sloppy',
                            'from' => $network['from'],'to' => $network['to'], 'quick' => false,
                            'label' => "pass traffic between statically routed subnets"),
                      $defaults['pass']
                    );
                }
            }
        }
    }
}
