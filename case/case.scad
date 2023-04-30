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

BATTERY_BOX_X = 45 + 2 + 2;
BATTERY_BOX_Y = 75 + 2;
BATTERY_BOX_Z = 13;

LID_BATTERY_BOX_X = BATTERY_BOX_X + 1.6 + 1.6 + 0.1 + 0.1;
LID_BATTERY_BOX_Y = BATTERY_BOX_Y + 1.6 + 1.6 + 0.1 + 0.1;
LID_BATTERY_BOX_Z = BATTERY_BOX_Z + 1.2;


module pcb() {
    cube([PCB_SIZE_X, PCB_SIZE_Y, PCB_SIZE_Z+1]);
    // PCB the SMD stuff on front
    
    cube([22.25, PCB_SIZE_Y-2.25, PCB_SIZE_Z + PCB_SIZE_PARTS_Z+1]);
    translate([0, -5, 0]) cube([22.5, 5, PCB_SIZE_Z + PCB_SIZE_PARTS_Z+1]);
}

module inlay() {
	difference(){
        union() {
            cylinder(h=INLAY_HEIGHT_SPACING_Y, r=7.5, $fn=180);
            translate([0, 0, 1.5]) {
                cylinder(h=INLAY_HEIGHT_HOLDER_MAGNET_Y, r=12, $fn=180);
            }
        }
		cube([VIERKANT, VIERKANT, 10], center=true);
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

module lid() {

    difference() {
        cube([LID_BATTERY_BOX_X, LID_BATTERY_BOX_Y, LID_BATTERY_BOX_Z]);
        translate([1.6 - 0.1, 1.6 - 0.1, 1.2]) {
            cube([BATTERY_BOX_X + 0.1 + 0.1, BATTERY_BOX_Y + 0.1 + 0.1, 20]);
        }
        translate([(LID_BATTERY_BOX_X/2 - (CASE_SIZE_X + 2.4)/2), LID_BATTERY_BOX_Y-2, LID_BATTERY_BOX_Z - (CASE_SIZE_Z + 1.2) ]) {
            cube([CASE_SIZE_X + 2.4, 20, 20]);
        }
    }
}

module box_pcb_battery() {
    // innen: 45mm x 75mm
    difference() {
        cube([BATTERY_BOX_X, BATTERY_BOX_Y, LID_BATTERY_BOX_Z]);
        translate([2, 2, 1]) {
            cube([45, 73, LID_BATTERY_BOX_Z]);
        }
        translate([BATTERY_BOX_X/2 - PCB_SIZE_X/2, BATTERY_BOX_Y - 5, 0]) {
            cube([PCB_SIZE_X+2, 6, 2]);
        }
        translate([-1, 20, 5]) {
            cube([100, 5, 2.2]);
        }
        
    }
    translate([BATTERY_BOX_X/2, BATTERY_BOX_Y, 0]) {
        translate([15+0.1, 0, 0]) cube([3-0.2, 20-0.1, 2]);
        translate([-19+0.1, 0, 0]) cube([2-0.2, 20-0.1, 2]);
    }
    
}


module case_top() {
    difference() {
        union() {
            translate([0, CASE_SIZE_Y/2, 0]) {
                cylinder(h=CASE_SIZE_Z + 1.2, d=CASE_SIZE_X + 2.4, $fn=190);
            }
            translate([-(CASE_SIZE_X/2)-1.2, -(CASE_SIZE_Y/2) -5 , 0]) {
                cube([CASE_SIZE_X + 2.4, CASE_SIZE_Y+5, CASE_SIZE_Z + 1.2]);
            }
        }
        
        // Bohrung Mitte
        translate([0, 0, -1]) cylinder(h=10, d=25, $fn=180);
        // Bohrung Oben
        translate([0, BOHRUNG_ABSTAND_MITTE, -1]) cylinder(h=10, d=BOHRUNGEN, $fn=180);
        // Bohrung Unten
        translate([0, - BOHRUNG_ABSTAND_MITTE, -1]) cylinder(h=10, d=BOHRUNGEN, $fn=180);


        translate([-15.5, -34, -1]) {
            pcb();
        }
        
        // Ankerpunkte Batterie PCB Box
        translate([0, -(CASE_SIZE_Y/2) -5, 0]) {
            translate([15, 0, 0]) cube([3, 20, 2 +0.1]);
            translate([-19, 0, 0]) cube([2, 20, 2+0.1]);
        }
    }
    
    translate([-15.5, -34, 0]) {
        translate([0, 0, PCB_SIZE_Z]) {
            cube([5, 21, PCB_SIZE_PARTS_Z]);
        }
    }
    // haltepunkte PCB
    translate([-13, 13, 0]) {
        cylinder(h=CASE_SIZE_Z, d=1.9, $fn=180);
        translate([0, 0, PCB_SIZE_Z]) {
            cylinder(h=CASE_SIZE_Z-PCB_SIZE_Z, d=5, $fn=190);
            translate([-5, -2.5, 0]) cube([5, 5, CASE_SIZE_Z-PCB_SIZE_Z]);
        }
    }
    translate([-13, -13, 0]) {
        cylinder(h=CASE_SIZE_Z, d=1.9, $fn=180);
        translate([0, 0, PCB_SIZE_Z]) {
            cylinder(h=CASE_SIZE_Z-PCB_SIZE_Z, d=5, $fn=180);
            
            translate([-5, -2.5, 0]) cube([5, 5, CASE_SIZE_Z-PCB_SIZE_Z]);
        }
    }

}



translate([50, 10, 0]) {
    inlay();
}

translate([0, 10, 0]) {
    case_top();
}


translate([100, 0, 0]) {
    box_pcb_battery();
}

translate([155, -1.6, 0]) {
    lid();
}
