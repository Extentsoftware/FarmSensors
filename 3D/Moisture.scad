use <threads.scad.txt>

//$fa = 0.5;
//$fs = 5;

shaftRadius=20;
shaftLength=40;
shaftInset=2;
shaftBore=14;
shimLength=5;


//translate([0,0,10])
//{
//    BrassObject();
//}

//translate([0,0,-7])
//{
    //BrassObject();
//}

MainShaft();
//translate([0,0,shimLength])
//metric_thread (shaftBore*2 , 1, shimLength, rectangle = 1);

module MainShaft() {
        translate([0,0,-shaftLength/2])
        {
            difference() {        
                MainShaft_1();
                cylinder(h=shaftLength*2, r=shaftBore, center=true);

                translate([0,0,shimLength*3])
                render() metric_thread (shaftBore*2 , 1, shimLength, internal=true);

            }    
        };
}

module MainShaft_1() {
    color("lime") 
    metric_thread( diameter=shaftRadius, pitch=1, length=shaftLength - shimLength);
    
    color("Blue") 
    cylinder(h=shaftLength, r=shaftRadius-shaftInset, center=true);
    
    translate([0,0,-shimLength])    
    color("green") cylinder(h=shaftLength - shimLength, r=shaftRadius, center=true);
 }
 
/*
module BrassObject() {
    difference() {
        BrassObject_1();
        cylinder(h=20, r=shaftRadius, center=true);
    }
}

module BrassObject_1() {
    color("gold") 
    rotate_extrude()
    translate([10,0])
    resize([5,10,10])
    circle(d=20);    
}
*/