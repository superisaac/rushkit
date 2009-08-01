/*
 * This file is part of Rushkit
 * Rushkit is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * Rushkit is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with Rushkit.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Copyright 2009 Zeng Ke
 * Email: superisaac.ke@gmail.com
 */

class App {
    static var app : App;

    var _windowDesk:MovieClip;

    static function mytrace(str:String)
    {
	var preTxt = App.app._windowDesk.debug.text;
	App.app._windowDesk.debug.text = str + "\n" + preTxt;
	//flash.external.ExternalInterface.call("debug_flv", str);
    }

    static function setupNetwork() {
	var sa = _root._url.split("/");
	_root._server = sa[2];
	var ss = _root._server.split(":");
	_root._host = ss[0];

	var sport = 1935;
	_root._rtmp_port = sport;	
	_root._rtmp_url = "rtmp://" + _root._host + ":" + _root._rtmp_port;

	_root.main_nc = new NetConnection();
	_root.main_nc.greeting = function(word) {
	    trace("greeting from server: " + word);
	};
	_root.main_nc.onStatus = function(info) {
	    trace("rtmp nc " + info.code);
	    if(info.code == 'NetConnection.Connect.Success') {
		var addObj = new Object();
		addObj.onResult = function(res) {
		  trace("result is " + res);
		};
		trace("call connection");

		setInterval(function() {
			_root.main_nc.call("add", addObj, 3, 5);
		}, 3000);
	    }
	}
	_root.main_nc.connect(_root._rtmp_url);
    }

    function connectionEstablished()
    {

    }
    function connectionClosed(){
	
    }
    function initialize() {
	var app = App.app;

	_windowDesk = _root.attachMovie("windowDesk", "_windowDesk", 3);
	app._windowDesk.clear();
	_windowDesk.createTextField("debug", 9, 0, 200, 300, 200);
	_windowDesk.debug.wordWrap = true;
	_windowDesk.debug.multiline = true;	
	_windowDesk.debug.textColor = 0x1e90ff;
	_windowDesk.debug.text = "";

	App.setupNetwork();
    }

    function onResize()
    {
    }

    static function setup()
    {
	app = new App();
	app.initialize();
	var this_app = app;
	var resizeL = new Object();
	resizeL.onResize= function()
	{
	    _root.clear();
	    this_app.onResize();
	}
	Stage.addListener(resizeL);
    }

    // entry point
    static function main(){
            System.security.allowDomain("10.0.0.5");
	Stage.scaleMode = "noscale";
	Stage.align = "tl";
	
	App.setup();
    }
}
