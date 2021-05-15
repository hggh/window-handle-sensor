$fn = 190;

module holder() {
    difference() {
        union() {
            cylinder(d=6, h=6);
            cylinder(d=3, h=7.5);
        }
        
        translate([0, 0, 5]) {
            cylinder(d=1.5, h=10);
        }
    }
}

module pin() {
    cylinder(h=2, d=4);
    cylinder(d=1.1, h=4);
}


translate([-15.5, -19, -1]) {
    cube([31, 38, 1.8]);
}
translate([9, 14, 0]) holder();
translate([9, -14, 0]) holder();
translate([-9, 14, 0]) holder();
translate([-9, -14, 0]) holder();

translate([0, 30, 0]) pin();