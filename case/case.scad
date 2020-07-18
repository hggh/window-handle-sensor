VIERKANT = 7.5;
CASE_SIZE_X = 40;
CASE_SIZE_Y = 68;
BOHRUNG_ABSTAND_MITTE = 21.75;
BOHRUNGEN = 10.75;

PCB_SIZE_X = 23.5;
PCB_SIZE_Y = 50;
PCB_SIZE_Z = 0.6 + 0.1;
PCB_SIZE_PARTS_Z = 1.2;

INLAY_HEIGHT_SPACING_Y = 1.4;
INLAY_HEIGHT_HOLDER_MAGNET_Y = 1.2;

CASE_SIZE_Z = PCB_SIZE_Z + INLAY_HEIGHT_SPACING_Y + INLAY_HEIGHT_HOLDER_MAGNET_Y;

module pcb() {
    cube([PCB_SIZE_X, PCB_SIZE_Y, PCB_SIZE_Z]);
    // PCB the SMD stuff on front
    cube([22.25, PCB_SIZE_Y-2.25, PCB_SIZE_Z + PCB_SIZE_PARTS_Z]);
}

module inlay() {
	difference(){
        union() {
            cylinder(h=INLAY_HEIGHT_SPACING_Y, r=7.5, $fn=180);
            translate([0, 0, 1.5]) {
                cylinder(h=INLAY_HEIGHT_HOLDER_MAGNET_Y, r=12, $fn=180);
            }
        }
		cube([(VIERKANT), (VIERKANT), 10], center=true);
		translate([10.4, 0, -1]) {
			cylinder(h=20, d=2, $fn=180);
		}
        translate([10.0, 2.8, -1]) {
			cylinder(h=20, d=2, $fn=180);
		}
        translate([10.0, -2.8, -1]) {
			cylinder(h=20, d=2, $fn=180);
		}
	}
}

module box() {
    difference() {
        translate([-(CASE_SIZE_X/2), -(CASE_SIZE_Y/2), 0]) {
            cube([CASE_SIZE_X, CASE_SIZE_Y, CASE_SIZE_Z]);
        }
        // Bohrung Mitte + inlay
        translate([0, 0, -1]) {
            cylinder(h=10, d=25, $fn=180);
        }         
        // Bohrung Oben
        translate([0, BOHRUNG_ABSTAND_MITTE, -1]) {
            cylinder(h=10, d=BOHRUNGEN, $fn=180);
        }
        // Bohrung Oben
        translate([0, - BOHRUNG_ABSTAND_MITTE, -1]) {
            cylinder(h=10, d=BOHRUNGEN, $fn=180);
        }
        // pcb
        translate([-15.5, -34, 0]) {
            pcb();
        }
    }
    // pcb holder
    translate([-15.5, -34, 0]) {
        translate([0, 0, PCB_SIZE_Z]) {
            cube([5, 11, PCB_SIZE_PARTS_Z]);
        }
    }
    // haltepunkte PCB
    translate([-13, 13, 0]) {
        cylinder(h=CASE_SIZE_Z, d=1.9, $fn=180);
        translate([0, 0, PCB_SIZE_Z]) {
            cylinder(h=CASE_SIZE_Z-PCB_SIZE_Z, d=5, $fn=180);
        }
    }
    translate([-13, -13, 0]) {
        cylinder(h=CASE_SIZE_Z, d=1.9, $fn=180);
        translate([0, 0, PCB_SIZE_Z]) {
            cylinder(h=CASE_SIZE_Z-PCB_SIZE_Z, d=5, $fn=180);
        }
    }
}


box();


translate([100, 10, 0]) {
    inlay();
}