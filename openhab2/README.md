### OpenHAB2 integration

OpenHAB2 2.4 and later are required!

### install plugins

for the OpenHAB2 integration you will need to install the OpenHAB2 plugins (e.g. via paperui)

* MQTT Binding

#### install files into OpenHAB2 installation

```
cp *.items items/
cp *.map transform/
cp *.rules rules/
cp *.things things/
 ```
 
#### configuration

open **rules/window-state.things** and configure *host*, *username* and *password* parameter.

For every new Window state device you need:
 
 * **rules/window-state.things**: copy the "Thing topic window1" block and count the number "1" up. Also replace the "1" in the *stateTopic*!
 * **items/window-state.items**: copy the last three lines and also count the number "1" up. Do not rename the Item names. They are needed for the rules. But update the label of the item.
 
 The rules are based on the Item Groups - every Item that is also placed in the same groups all alarm rules will work on the fly! 
 
 Inside the **rules(window-state.rules** file you can configure how you will be notified if the battery is low or the device was not active within the last 25 hours.
 See line 49 and 63 inside the rules file. If you want to use Telegram, please install the Telegram plugin and configure your bot and enable the lines.
