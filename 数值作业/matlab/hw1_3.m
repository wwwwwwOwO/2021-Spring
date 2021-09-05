TOL = 1e-8;

ga = @(x) (4*x^5+1)/(5*x^4+1);
xa = 0.5;
ka = 0;
while abs(ga(xa)-xa) > TOL
    xa = ga(xa);
    ka = ka+1;
end
display(ka);
display(ga(xa));

gb = @(x) (x*cos(x)-sin(x)+5)/(cos(x)-6);
xb = -0.5;
kb = 0;
while abs(gb(xb)-xb) > TOL
    xb = gb(xb);
    kb = kb+1;
end
display(kb);
display(gb(xb));

gc = @(x) (-x^3+2*x^2+4*x-x*log(x))/(2*x+1);
xc = 1;
kc = 0;
while abs(gc(xc)-xc) > TOL
    xc = gc(xc);
    kc = kc+1;
end
display(kc);
display(gc(xc));

