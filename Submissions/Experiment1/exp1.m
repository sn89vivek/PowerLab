y=0:0.1:20.5;
% y=y./36;
I0 = 4.447;
n=36;
Rp=1653.84/(36*36);
Vt=26e-3;
Rs=0.0241;
Id0=1.3e-09;
syms x;
Vd=zeros(1,length(y));
Icell=zeros(1,length(y));
for i = 1:length(y)
    %Vd(i) = double(solve((-I0*Rs+Rs*Id0*exp(x/Vt)+Rs*Id0+x*(Rs/Rp+1))==y(i)));
    %Icell(i) = (Vd(i)-y(i))/Rs;
   Icell(i) = double(solve((I0 - Id0*(exp((y(i)+x*n*Rs)/(n*Vt))-1) - ((y(i)+x*n*Rs)/(n*Rp)))==x));
end

% %%
% Rp1=Rp/(36*36);
% syms x1;
% Vd1=zeros(1,length(y));
% Icell1=zeros(1,length(y));
% for i = 1:length(y)
%     Vd1(i) = double(solve((-I0*Rs+Rs*Id0*exp(x1/Vt)+Rs*Id0+x1*(Rs/Rp1+1))==y(i)));
%     Icell1(i) = (Vd1(i)-y(i))/Rs;
%    
% end
plot(y,Icell)