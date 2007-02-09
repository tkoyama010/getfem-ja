clear pde;
gf_workspace('clear all');

% set a custom colormap
r=[0.7 .7 .7]; l = r(end,:); s=63; s1=20; s2=25; s3=48;s4=55; for i=1:s, c1 = max(min((i-s1)/(s2-s1),1),0);c2 = max(min((i-s3)/(s4-s3),1),0); r(end+1,:)=(1-c2)*((1-c1)*l + c1*[1 0 0]) + c2*[1 .8 .2]; end; colormap(r);


incompressible = 1

if 0,
  h=20
  % import the mesh
  %m=gfMesh('load', 'ladder.mesh');
  %m=gfMesh('load', 'ladder_1500.mesh');
  m=gfMesh('load', 'holed_bar.mesh');
  set(m, 'transform', [1 0 0; 0 0 1; 0 1 0]);
  mfu=gfMeshFem(m,3);     % mesh-fem supporting a 3D-vector field
  mfd=gfMeshFem(m,1);     % scalar mesh_fem
  % the mesh_im stores the integration methods for each tetrahedron
  mim=gfMeshIm(m,gfInteg('IM_TETRAHEDRON(5)'));
  % we choose a P2 fem for the main unknown
  set(mfu, 'fem',gfFem('FEM_HERMITE(3)'));
  %set(mfu, 'fem',gfFem('FEM_PK(3,2)'));
  mfdu=gfMeshFem(m,1);
  % the material is homogeneous, hence we use a P0 fem for the data
  gf_mesh_fem_set(mfd,'fem',gf_fem('FEM_PK(3,1)'));
  % the P2 fem is not derivable across elements, hence we use a discontinuous
  % fem for the derivative of U.
  gf_mesh_fem_set(mfdu,'fem',gf_fem('FEM_PK_DISCONTINUOUS(3,2)'));
else
  N1=1; N2=4; h=20
  m=gfMesh('cartesian',(0:N1)/N1 - .5, (0:N2)/N2*h, ((0:N1)/N1 - .5)*3);
  mfu=gfMeshFem(m,3);     % mesh-fem supporting a 3D-vector field
  mfd=gfMeshFem(m,1);     % scalar mesh_fem
  % the mesh_im stores the integration methods for each tetrahedron
  mim=gfMeshIm(m,gfInteg('IM_GAUSS_PARALLELEPIPED(3,6)'));
  % we choose a P2 fem for the main unknown
  set(mfu, 'fem',gfFem('FEM_QK(3,2)'));
  mfdu=gfMeshFem(m,1);
  % the material is homogeneous, hence we use a P0 fem for the data
  gf_mesh_fem_set(mfd,'fem',gf_fem('FEM_QK(3,1)'));
  % the P2 fem is not derivable across elements, hence we use a discontinuous
  % fem for the derivative of U.
  gf_mesh_fem_set(mfdu,'fem',gf_fem('FEM_QK_DISCONTINUOUS(3,2)'));
end;

m_char=get(m, 'char');
mfu_char=get(mfu, 'char');
mfdu_char=get(mfdu, 'char');

% display some informations about the mesh
disp(sprintf('nbcvs=%d, nbpts=%d, nbdof=%d',get(m,'nbcvs'),...
             get(m,'nbpts'),get(mfu,'nbdof')));
P=get(m,'pts'); % get list of mesh points coordinates
%pidtop=find(abs(P(2,:)-13)<1e-6); % find those on top of the object
%pidbot=find(abs(P(2,:)+10)<1e-6); % find those on the bottom

pidtop=find(abs(P(2,:)-h)<1e-6); % find those on top of the object
pidbot=find(abs(P(2,:)-0)<1e-6); % find those on the bottom


% build the list of faces from the list of points
ftop=get(m,'faces from pid',pidtop); 
fbot=get(m,'faces from pid',pidbot);
% assign boundary numbers
set(m,'boundary',1,ftop);
set(m,'boundary',2,fbot);
set(m,'boundary',3,[ftop fbot]);



if ~incompressible,
  b0=gfMdBrick('nonlinear_elasticity', mim, mfu, 'Ciarlet Geymonat');
  b1=b0;
  set(b1, 'param', 'params', [1;1;-1.4]);
else
  b0=gfMdBrick('nonlinear_elasticity', mim, mfu, 'Mooney Rivlin');
  mfp = gfMeshFem(m,1); 
  set(mfp, 'classical discontinuous fem', 1);
  b1=gfMdBrick('nonlinear_elasticity_incompressibility_term',b0,mfp);
end
%b2=gfMdBrick('dirichlet', b1, 2);
b3=gfMdBrick('dirichlet', b1, 3, mfu, 'penalized');

mds=gfMdState(b3)

VM=zeros(1,get(mfdu,'nbdof'));

reload = 0;

if (reload == 0),
  UU=[];
  VVM=[];
  nbstep=40
else
  load 'demo_nonlinear_elasticity_U.mat';
  nb_step = size(UU,1);
end;
P=get(mfd, 'dof_nodes');
r = sqrt(P(1 ,:).^2 + P(3, :).^2);
theta = atan2(P(3,:),P(1,:));



for step=1:nbstep,
  w = 3*step/nbstep;
  %set(b2, 'param', 'R', [0;0;0]);

  if (~reload),
    R=zeros(3, get(mfd, 'nbdof'));
    dtheta =  pi;
    dtheta2 = pi/2;
    
    i_top = get(mfd, 'dof on boundary', 1);
    i_bot = get(mfd, 'dof on boundary', 2);
    dd = max(P(1,i_top)*sin(w*dtheta));
    if (w < 1), 
      RT1 = axrot_matrix([0 h*.75 0], [0 h*.75 1], w*dtheta);
      RT2 = axrot_matrix([0 0 0], [0 1 0], sqrt(w)*dtheta2);
      RB1 = axrot_matrix([0 h*.25 0], [0 h*.25 1], -w*dtheta);
      RB2 = RT2';
    elseif (w < 2),
      RT1 = axrot_matrix([0 h*.75 0], [0 h*.75 1], (2-w)*dtheta);
      RT2 = axrot_matrix([0 0 0], [0 1 0], w*dtheta2);
      RB1 = axrot_matrix([0 h*.25 0], [0 h*.25 1], -(2-w)*dtheta);
      RB2 = RT2';
    else
      RT1 = axrot_matrix([0 h*.75 0], [0 h*.75 1], 0);
      RT2 = axrot_matrix([0 0 0], [0 1 0], (3-w)*2*dtheta2);
      RB1 = axrot_matrix([0 h*.25 0], [0 h*.25 1], 0);
      RB2 = RT2';    
    end;
    for i=i_top,
      ro = RT1*RT2*[P(:,i);1];
      R(:, i) = ro(1:3) - P(:,i);
    end
    for i=i_bot,
      ro = RB1*RB2*[P(:,i);1];
      R(:, i) = ro(1:3) - P(:,i);
    end
    set(b3, 'param', 'R', mfd, R);
    get(b3, 'solve', mds, 'very noisy', 'max_iter', 100, 'max_res', 1e-5);
    U=get(mds, 'state'); U=U(1:get(mfu, 'nbdof'));
    VM = get(b0, 'von mises', mds, mfdu);
    UU = [UU;U]; 
    VVM = [VVM;VM];
    save demo_nonlinear_elasticity_U.mat UU VVM m_char mfu_char mfdu_char;
  else
    U=UU(step,:);
    VM=VVM(step,:);
  end;
  disp(sprintf('step %d/%d : |U| = %g',step,nbstep,norm(U)));

  gf_plot(mfdu,VM,'mesh','off', 'cvlst',gf_mesh_get(mfdu,'outer faces'), 'deformation',U,'deformation_mf',mfu,'deformation_scale', 1, 'refine', 8); colorbar;
  axis([-3     6     0    20    -2     2]); caxis([0 .3]);
  view(30+20*w, 23+30*w);  
  campos([50 -30 80]);
  camva(8);
  camup
  camlight; 
  axis off;
  pause(1); 
  % save a picture..
  %print(gcf, '-dpng', '-r150', sprintf('torsion%03d',step));
end;
  
disp('end of computations, you can now replay the animation with')
disp('demo_nonlinear_elasticity_anim')

