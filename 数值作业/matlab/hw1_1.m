x = 1:14;
x = 10.^(-x);
res1 = (1-sec(x))./((tan(x)).^2);
res2 = -1./(sec(x)+1);
display(res1);
display(res2);

res3 = (1-(1-x).^3)./x;
res4 = x.^2-3.*x+3;
display(res3);
display(res4);
