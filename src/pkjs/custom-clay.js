// Farbvorlagen fuer die Clay-Seite: die Auswahl unter "Template" setzt alle
// Farbfelder auf einmal. Wer danach eine einzelne Farbe aendert, landet bei
// "Custom" -- die Vorlage ist nur ein Startpunkt.
//
// Clay schickt diese Funktion als Quelltext an die Konfigseite, deshalb muss
// alles, was sie braucht, in ihr selbst stehen (kein require).

module.exports = function() {
  var clayConfig = this;

  // Reihenfolge der Spalten = COLOR_KEYS
  var COLOR_KEYS = ['COLOR_BG', 'COLOR_DIAL', 'COLOR_HOUR', 'COLOR_MINUTE',
                    'COLOR_HUB', 'COLOR_TEXT', 'COLOR_ACCENT',
                    'COLOR_MOON_LIT', 'COLOR_MOON_DARK'];

  //              bg        dial      hour      minute    hub       text      accent    moon lit  moon dark
  var TEMPLATES = {
    classic: ['FFFFFF', '000000', '000000', '000000', '000000', '000000', '000000', 'FFFFFF', '000000'],
    noir:    ['000000', 'FFFFFF', 'FFFFFF', 'FFFFFF', 'FFFFFF', 'FFFFFF', 'FFFFFF', 'FFFFFF', '000000'],
    swiss:   ['FFFFFF', '000000', '000000', 'FF0000', 'FF0000', '000000', 'FF0000', 'FFFFFF', '000000'],
    ivory:   ['FFFFAA', '000000', '000000', 'AA5500', 'AA5500', '555555', 'AA5500', 'FFFFFF', '555555'],
    blush:   ['FFAAAA', '000000', '000000', 'AA0055', 'AA0055', '000000', 'AA0055', 'FFFFFF', '000000'],
    navy:    ['000055', 'FFFFFF', 'FFFFFF', '55AAFF', '55AAFF', 'FFFFFF', '55AAFF', 'FFFFAA', '0000AA'],
    ocean:   ['0055AA', 'FFFFFF', 'FFFFFF', 'AAFFFF', 'FFFFFF', 'FFFFFF', 'AAFFFF', 'FFFFFF', '000055'],
    forest:  ['005500', 'FFFFFF', 'FFFFFF', 'AAFF55', 'AAFF55', 'FFFFFF', 'AAFF55', 'FFFFFF', '000000'],
    plum:    ['550055', 'FFFFFF', 'FFFFFF', 'FFAAFF', 'FFAAFF', 'FFFFFF', 'FFAAFF', 'FFFFFF', '000000'],
    sport:   ['000000', 'FFFFFF', 'FFFFFF', 'FF5500', 'FF5500', 'AAAAAA', 'FF5500', 'FFFFFF', '000000']
  };

  clayConfig.on(clayConfig.EVENTS.AFTER_BUILD, function() {
    var template = clayConfig.getItemByMessageKey('TEMPLATE');
    if (!template) { return; }
    var applying = false;

    template.on('change', function() {
      var colors = TEMPLATES[template.get()];
      if (!colors) { return; }
      applying = true;
      for (var i = 0; i < COLOR_KEYS.length; i++) {
        var item = clayConfig.getItemByMessageKey(COLOR_KEYS[i]);
        if (item) { item.set(parseInt(colors[i], 16)); }
      }
      applying = false;
    });

    COLOR_KEYS.forEach(function(key) {
      var item = clayConfig.getItemByMessageKey(key);
      if (!item) { return; }
      item.on('change', function() {
        if (!applying && template.get() !== 'custom') { template.set('custom'); }
      });
    });
  });
};
