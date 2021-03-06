'''Copyright (c) 2020 James Gayvert
    
    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:
    
    The above copyright notice and this permission notice shall be included in all
    copies or substantial portions of the Software.
    
    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
    SOFTWARE.'''

import pyopencap
from pyscf import gto, scf, fci, tools
import numpy as np
import os
import sys

destDir="../opencap/tests/data"
sys_dict = {"geometry":'''H        0.0     0.0     0.54981512
    H        0.0     0.0     -0.54981512
    X       0.0     0.0     0.0''',
        "basis_file":destDir+"/test_basis_for_pyscf.bas",
        "molecule": "inline",
        "cart_bf" : "dfg"
}
cap_dict = {
    "cap_type": "box",
        "cap_x":"6.00",
            "cap_y":"6.00",
            "cap_z":"6.7",
            "Radial_precision": "14",
            "angular_points": "110"
}
sys_dict2 = { "molecule": "molden", "basis_file": destDir+"/molden_file_cart.molden"}

s = pyopencap.System(sys_dict)
H_bas = gto.basis.load(destDir+'/pyscf_test_basis.bas', 'H')
X_bas = gto.basis.load(destDir+'/pyscf_test_basis.bas', 'O')
mol = gto.M(
            atom = 'H        0.0     0.0     0.54981512;\
            H        0.0     0.0     -0.54981512;\
            X       0.0     0.0     0.0',
            basis = {'H': H_bas, 'X':X_bas}, cart=True
            )
mol.build()
myhf = scf.RHF(mol)
myhf.kernel()

def test_overlap():
    basis_id_str = s.get_basis_ids()
    pyscf_smat = scf.hf.get_ovlp(mol)
    s.check_overlap_mat(pyscf_smat,"pyscf")

def test_from_molden():
    tools.molden.from_scf(myhf,destDir+"/molden_file_cart.molden")
    s2 = pyopencap.System(sys_dict2)
    pyscf_smat = scf.hf.get_ovlp(mol)
    s2.check_overlap_mat(pyscf_smat,"pyscf")
    os.remove(destDir+"/molden_file_cart.molden")

def test_pyscf():
    pc = pyopencap.CAP(s,cap_dict,3,"pyscf")
    fs = fci.FCI(mol, myhf.mo_coeff)
    fs.nroots = 3
    e, c = fs.kernel()
    for i in range(0,len(fs.ci)):
        for j in range(0,len(fs.ci)):
            dm1 = fs.trans_rdm1(fs.ci[i],fs.ci[j],myhf.mo_coeff.shape[1],mol.nelec)
            dm1_ao = np.einsum('pi,ij,qj->pq', myhf.mo_coeff, dm1, myhf.mo_coeff.conj())
            pc.add_tdm(dm1_ao,i,j,"pyscf")
    pc.compute_ao_cap()
    pyscf_smat = scf.hf.get_ovlp(mol)
    pc.renormalize_cap(pyscf_smat,"pyscf")
    pc.compute_perturb_cap()
    mat=pc.get_perturb_cap()
