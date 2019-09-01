from setuptools import setup, Extension
from setuptools.command.build_ext import build_ext
import sys
import setuptools
from glob import glob

__version__ = '0.0.1'


class get_pybind_include(object):
    """Helper class to determine the pybind11 include path

    The purpose of this class is to postpone importing pybind11
    until it is actually installed, so that the ``get_include()``
    method can be invoked. """

    def __init__(self, user=False):
        self.user = user

    def __str__(self):
        import pybind11
        return pybind11.get_include(self.user)


def find_boom_root(dirpath):
    if os.path.basename(dirpath) == "BOOM":
        return dirpath
    elif dirpath == "/":
        raise Exception(f"Cannot find BOOM on path {dirpath}.")
    else:
        return find_boom_root(os.path.dirname(dirpath))
    
BOOM = find_boom_root(os.path.getcwd())
    
boom_headers = glob(f"{BOOM}/*.hpp")

distributions_sources = glob(f"{BOOM}/distributions/*.cpp")
distributions_headers = (
    [f"{BOOM}/distributions.hpp"]
    + glob(f"{BOOM}/distributions/*.hpp")
    )

linalg_sources = glob(f"{BOOM}/LinAlg/*.cpp")
linalg_headers = glob(f"{BOOM}/LinAlg/*.hpp")

math_sources = glob(f"{BOOM}/math/*.cpp") + glob(f"{BOOM}/math/cephes/*.cpp")

numopt_sources = glob(f"{BOOM}/numopt/*.cpp")
numopt_headers = ["{BOOM}/numopt.hpp"] + glob(f"{BOOM}/numopt/*.hpp")

rmath_sources = glob(f"{BOOM}/Bmath/*.cpp")
rmath_headers = glob(f"{BOOM}/Bmath/*.hpp")

samplers_sources = glob(f"{BOOM}/Samplers/*.cpp")
samplers_headers = glob(f"{BOOM}/Samplers/*.hpp")

stats_sources = glob(f"{BOOM}/stats/*.cpp")
stats_headers = glob(f"{BOOM}/stats/*.hpp")

targetfun_sources = glob(f"{BOOM}/TargetFun/*.cpp")
targetfun_headers = glob(f"{BOOM}/TargetFun/*.hpp")

utils_sources = glob(f"{BOOM}/cpputil/*.cpp")
utils_headers = glob(f"{BOOM}/cpputil/*.hpp")

models_sources = (
    glob(f"{BOOM}/Models/*.cpp")
    + glob(f"{BOOM}/Models/PosteriorSamplers/*.cpp")
    + glob(f"{BOOM}/Models/Policies/*.cpp"))
models_headers = (
    glob(f"{BOOM}/Models/*.hpp")
    + glob(f"{BOOM}/Models/Policies/*.hpp")
    + glob(f"{BOOM}/Models/PosteriorSamplers/*.hpp"))

# Specific model classes to be added later, glm's hmm's, etc.

boom_library_sources = (
    distributions_sources
    + linalg_sources
    + math_sources
    + numopt_sources
    + rmath_sources
    + samplers_sources
    + stats_sources
    + targetfun_sources
    + utils_sources
    + models_sources)

boom_extension_sources = ["./Models/GaussianModel.cpp"]

boom_sources = boom_library_sources + boom_extension_sources

ext_modules = [
    Extension(
        'Boom',
        sources=boom_sources,
        include_dirs=[
            "{BOOM}/../..",
            # Path to pybind11 headers
            get_pybind_include(),
            get_pybind_include(user=True)
        ],
        language='c++'
    ),
]


# As of Python 3.6, CCompiler has a `has_flag` method.
# cf http://bugs.python.org/issue26689
def has_flag(compiler, flagname):
    """Return a boolean indicating whether a flag name is supported on
    the specified compiler.
    """
    import tempfile
    with tempfile.NamedTemporaryFile('w', suffix='.cpp') as f:
        f.write('int main (int argc, char **argv) { return 0; }')
        try:
            compiler.compile([f.name], extra_postargs=[flagname])
        except setuptools.distutils.errors.CompileError:
            return False
    return True


def cpp_flag(compiler):
    """Return the -std=c++[11/14/17] compiler flag.

    The newer version is prefered over c++11 (when it is available).
    """
    flags = ['-std=c++17', '-std=c++14', '-std=c++11']

    for flag in flags:
        if has_flag(compiler, flag): return flag

    raise RuntimeError('Unsupported compiler -- at least C++11 support '
                       'is needed!')


class BuildExt(build_ext):
    """A custom build extension for adding compiler-specific options."""
    c_opts = {
        'msvc': ['/EHsc'],
        'unix': [],
    }
    l_opts = {
        'msvc': [],
        'unix': [],
    }

    if sys.platform == 'darwin':
        darwin_opts = ['-stdlib=libc++', '-mmacosx-version-min=10.14']
        c_opts['unix'] += darwin_opts
        l_opts['unix'] += darwin_opts

    def build_extensions(self):
        ct = self.compiler.compiler_type
        opts = self.c_opts.get(ct, [])
        link_opts = self.l_opts.get(ct, [])
        if ct == 'unix':
            opts.append('-DVERSION_INFO="%s"' % self.distribution.get_version())
            opts.append(cpp_flag(self.compiler))
            if has_flag(self.compiler, '-fvisibility=hidden'):
                opts.append('-fvisibility=hidden')
        elif ct == 'msvc':
            opts.append('/DVERSION_INFO=\\"%s\\"' % self.distribution.get_version())
        for ext in self.extensions:
            ext.extra_compile_args = opts
            ext.extra_link_args = link_opts
        build_ext.build_extensions(self)

setup(
    name='BayesBoom',
    version=__version__,
    author='Steven L. Scott',
    author_email='steve.the.bayesian@gmail.com',
    url='https://github.com/steve-the-bayesian/BOOM',
    description='Tools for Bayesian modeling.',
    long_description="""Boom stands for 'Bayesian object oriented modeling'.  
    It is also the sound your computer makes when it crashes. 

    The main part of the Boom library is formulated in terms of abstractions for
    Model, Data, Params, and PosteriorSampler.  A Model is primarily an
    environment where parameters can be learned from data.  The primary learning
    method is Markov chain Monte Carlo, with custom samplers defined for
    specific models.

    The archetypal Boom program looks something like this:

    import BoomBayes as Boom

    some_data = 3 * np.random.randn(100) + 7
    model = Boom.GaussianModel()
    model.set_data(some_data)
    precision_prior = Boom.GammaModel(0.5, 1.5)
    mean_prior = Boom.GaussianModel(0, 10**2)
    poseterior_sampler = Boom.GaussianSemiconjugateSampler(
        model, mean_prior, precision_prior)
    model.set_method(poseterior_sampler)
    niter = 100
    mean_draws = np.zeros(niter)
    sd_draws = np.zeros(niter)
    for i in range(100):
        model.sample_posterior()
        mean_draws[i] = model.mu()
        sd_draws[i] = model.sigma()

    """,
    ext_modules=ext_modules,
    install_requires=['pybind11>=2.3'],
    setup_requires=['pybind11>=2.3'],
    cmdclass={'build_ext': BuildExt},
    zip_safe=False,
)